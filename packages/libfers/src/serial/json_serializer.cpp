// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
* @file json_serializer.cpp
* @brief Implements JSON serialization and deserialization for FERS objects.
*
* This file leverages the `nlohmann/json` library's support for automatic
* serialization via `to_json` and `from_json` free functions. By placing these
* functions within the namespaces of the objects they serialize, we enable
* Argument-Dependent Lookup (ADL), allowing the library to automatically find
* and use these functions for conversion. This approach keeps the serialization
* logic decoupled from the core object definitions.
*/

#include "serial/json_serializer.h"

#include <random>
#include <nlohmann/json.hpp>

#include <libfers/antenna_factory.h>
#include <libfers/coord.h>
#include <libfers/parameters.h>
#include <libfers/path.h>
#include <libfers/platform.h>
#include <libfers/receiver.h>
#include <libfers/rotation_path.h>
#include <libfers/target.h>
#include <libfers/transmitter.h>
#include <libfers/world.h>

#include "pulse_factory.h"
#include "signal/radar_signal.h"
#include "timing/prototype_timing.h"
#include "timing/timing.h"

// TODO: Add file path validation and error handling as needed.

namespace math
{
	void to_json(nlohmann::json& j, const Vec3& v) { j = {{"x", v.x}, {"y", v.y}, {"z", v.z}}; }

	void from_json(const nlohmann::json& j, Vec3& v)
	{
		j.at("x").get_to(v.x);
		j.at("y").get_to(v.y);
		j.at("z").get_to(v.z);
	}

	void to_json(nlohmann::json& j, const Coord& c)
	{
		j = {{"time", c.t}, {"x", c.pos.x}, {"y", c.pos.y}, {"altitude", c.pos.z}};
	}

	void from_json(const nlohmann::json& j, Coord& c)
	{
		j.at("time").get_to(c.t);
		j.at("x").get_to(c.pos.x);
		j.at("y").get_to(c.pos.y);
		j.at("altitude").get_to(c.pos.z);
	}

	void to_json(nlohmann::json& j, const RotationCoord& rc)
	{
		// The internal engine uses mathematical angles (radians, CCW from East),
		// but the UI and XML format use compass degrees (CW from North). This
		// conversion is performed at the serialization boundary for consistency.
		const RealType az_deg = std::fmod(90.0 - rc.azimuth * 180.0 / PI + 360.0, 360.0);
		const RealType el_deg = rc.elevation * 180.0 / PI;
		j = {{"time", rc.t}, {"azimuth", az_deg}, {"elevation", el_deg}};
	}

	void from_json(const nlohmann::json& j, RotationCoord& rc)
	{
		j.at("time").get_to(rc.t);
		const RealType az_deg = j.at("azimuth").get<RealType>();
		const RealType el_deg = j.at("elevation").get<RealType>();

		// Convert from compass degrees (from JSON/UI) to the internal engine's
		// mathematical angle representation (radians, CCW from East).
		rc.azimuth = (90.0 - az_deg) * (PI / 180.0);
		rc.elevation = el_deg * (PI / 180.0);
	}

	NLOHMANN_JSON_SERIALIZE_ENUM
	(Path::InterpType,
	 {{Path::InterpType::INTERP_STATIC, "static"},
	 {Path::InterpType::INTERP_LINEAR, "linear"},
	 {Path::InterpType::INTERP_CUBIC, "cubic"}}
		)

	void to_json(nlohmann::json& j, const Path& p)
	{
		j = {{"interpolation", p.getType()}, {"positionwaypoints", p.getCoords()}};
	}

	void from_json(const nlohmann::json& j, Path& p)
	{
		p.setInterp(j.at("interpolation").get<Path::InterpType>());
		const auto waypoints = j.at("positionwaypoints").get<std::vector<Coord>>();
		for (const auto& wp : waypoints) { p.addCoord(wp); }
		p.finalize();
	}

	NLOHMANN_JSON_SERIALIZE_ENUM
	(RotationPath::InterpType,
	 {{RotationPath::InterpType::INTERP_STATIC, "static"},
	 {RotationPath::InterpType::INTERP_CONSTANT, "constant"}, // Not used in xml_parser or UI yet, but for completeness
	 {RotationPath::InterpType::INTERP_LINEAR, "linear"},
	 {RotationPath::InterpType::INTERP_CUBIC, "cubic"}}
		)

	void to_json(nlohmann::json& j, const RotationPath& p)
	{
		j["interpolation"] = p.getType();
		if (p.getType() == RotationPath::InterpType::INTERP_CONSTANT)
		{
			// A constant-rate rotation path corresponds to the <fixedrotation> XML element.
			// The start and rate values are converted to compass degrees per second for UI consistency.
			const RealType start_az_deg = std::fmod(90.0 - p.getStart().azimuth * 180.0 / PI + 360.0, 360.0);
			const RealType start_el_deg = p.getStart().elevation * 180.0 / PI;
			const RealType rate_az_deg_s = -p.getRate().azimuth * 180.0 / PI; // Invert for CW rate
			const RealType rate_el_deg_s = p.getRate().elevation * 180.0 / PI;
			j["startazimuth"] = start_az_deg;
			j["startelevation"] = start_el_deg;
			j["azimuthrate"] = rate_az_deg_s;
			j["elevationrate"] = rate_el_deg_s;
		}
		else { j["rotationwaypoints"] = p.getCoords(); }
	}

	void from_json(const nlohmann::json& j, RotationPath& p)
	{
		p.setInterp(j.at("interpolation").get<RotationPath::InterpType>());
		const auto waypoints = j.at("rotationwaypoints").get<std::vector<RotationCoord>>();
		for (const auto& wp : waypoints) { p.addCoord(wp); }
		p.finalize();
	}

}

namespace timing
{
	void to_json(nlohmann::json& j, const PrototypeTiming& pt)
	{
		j = nlohmann::json{{"name", pt.getName()}, {"frequency", pt.getFrequency()},
		                   {"synconpulse", pt.getSyncOnPulse()}};

		if (pt.getFreqOffset().has_value()) { j["freq_offset"] = pt.getFreqOffset().value(); }
		if (pt.getRandomFreqOffsetStdev().has_value())
		{
			j["random_freq_offset_stdev"] = pt.getRandomFreqOffsetStdev().value();
		}
		if (pt.getPhaseOffset().has_value()) { j["phase_offset"] = pt.getPhaseOffset().value(); }
		if (pt.getRandomPhaseOffsetStdev().has_value())
		{
			j["random_phase_offset_stdev"] = pt.getRandomPhaseOffsetStdev().value();
		}

		std::vector<RealType> alphas;
		std::vector<RealType> weights;
		pt.copyAlphas(alphas, weights);
		if (!alphas.empty())
		{
			nlohmann::json noise_entries = nlohmann::json::array();
			for (size_t i = 0; i < alphas.size(); ++i)
			{
				noise_entries.push_back({{"alpha", alphas[i]}, {"weight", weights[i]}});
			}
			j["noise_entries"] = noise_entries;
		}
	}

	void from_json(const nlohmann::json& j, PrototypeTiming& pt)
	{
		pt.setFrequency(j.at("frequency").get<RealType>());
		if (j.value("synconpulse", false)) { pt.setSyncOnPulse(); }

		if (j.contains("freq_offset")) { pt.setFreqOffset(j.at("freq_offset").get<RealType>()); }
		if (j.contains("random_freq_offset_stdev"))
		{
			pt.setRandomFreqOffsetStdev(j.at("random_freq_offset_stdev").get<RealType>());
		}
		if (j.contains("phase_offset")) { pt.setPhaseOffset(j.at("phase_offset").get<RealType>()); }
		if (j.contains("random_phase_offset_stdev"))
		{
			pt.setRandomPhaseOffsetStdev(j.at("random_phase_offset_stdev").get<RealType>());
		}

		if (j.contains("noise_entries"))
		{
			for (const auto& entry : j.at("noise_entries"))
			{
				pt.setAlpha(entry.at("alpha").get<RealType>(), entry.at("weight").get<RealType>());
			}
		}
	}
}

namespace fers_signal
{
	void to_json(nlohmann::json& j, const RadarSignal& rs)
	{
		j = nlohmann::json{{"name", rs.getName()}, {"power", rs.getPower()}, {"carrier", rs.getCarrier()}};
		if (dynamic_cast<const CwSignal*>(rs.getSignal())) { j["type"] = "continuous"; }
		else
		{
			j["type"] = "file";
			if (const auto& filename = rs.getFilename(); filename.has_value()) { j["filename"] = *filename; }
		}
	}

	void from_json(const nlohmann::json& j, std::unique_ptr<RadarSignal>& rs)
	{
		const auto name = j.at("name").get<std::string>();
		const auto power = j.at("power").get<RealType>();
		const auto carrier = j.at("carrier").get<RealType>();
		const auto type = j.at("type").get<std::string>();

		if (type == "continuous")
		{
			auto cw_signal = std::make_unique<CwSignal>();
			rs = std::make_unique<RadarSignal>(name, power, carrier, params::endTime() - params::startTime(),
			                                   std::move(cw_signal));
		}
		else if (type == "file")
		{
			if (!j.contains("filename")) { throw std::runtime_error("File-based pulse requires a filename."); }
			const auto filename = j.at("filename").get<std::string>();
			rs = serial::loadPulseFromFile(name, filename, power, carrier);
		}
		else { throw std::runtime_error("Unsupported pulse type in from_json: " + type); }
	}
}

namespace antenna
{
	void to_json(nlohmann::json& j, const Antenna& a)
	{
		j = {{"name", a.getName()}, {"efficiency", a.getEfficiencyFactor()}};

		if (const auto* sinc = dynamic_cast<const Sinc*>(&a))
		{
			j["pattern"] = "sinc";
			j["alpha"] = sinc->getAlpha();
			j["beta"] = sinc->getBeta();
			j["gamma"] = sinc->getGamma();
		}
		else if (const auto* gaussian = dynamic_cast<const Gaussian*>(&a))
		{
			j["pattern"] = "gaussian";
			j["azscale"] = gaussian->getAzimuthScale();
			j["elscale"] = gaussian->getElevationScale();
		}
		else if (const auto* sh = dynamic_cast<const SquareHorn*>(&a))
		{
			j["pattern"] = "squarehorn";
			j["diameter"] = sh->getDimension();
		}
		else if (const auto* parabolic = dynamic_cast<const Parabolic*>(&a))
		{
			j["pattern"] = "parabolic";
			j["diameter"] = parabolic->getDiameter();
		}
		else if (const auto* xml = dynamic_cast<const XmlAntenna*>(&a))
		{
			j["pattern"] = "xml";
			j["filename"] = xml->getFilename();
		}
		else if (const auto* h5 = dynamic_cast<const H5Antenna*>(&a))
		{
			j["pattern"] = "file";
			j["filename"] = h5->getFilename();
		}
		else { j["pattern"] = "isotropic"; }
	}

	void from_json(const nlohmann::json& j, std::unique_ptr<Antenna>& ant)
	{
		const auto name = j.at("name").get<std::string>();
		const auto pattern = j.at("pattern").get<std::string>();

		if (pattern == "isotropic") { ant = std::make_unique<Isotropic>(name); }
		else if (pattern == "sinc")
		{
			ant = std::make_unique<Sinc>(name, j.at("alpha").get<RealType>(), j.at("beta").get<RealType>(),
			                             j.at("gamma").get<RealType>());
		}
		else if (pattern == "gaussian")
		{
			ant = std::make_unique<Gaussian>(name, j.at("azscale").get<RealType>(), j.at("elscale").get<RealType>());
		}
		else if (pattern == "squarehorn")
		{
			ant = std::make_unique<SquareHorn>(name, j.at("diameter").get<RealType>());
		}
		else if (pattern == "parabolic")
		{
			ant = std::make_unique<Parabolic>(name, j.at("diameter").get<RealType>());
		}
		else if (pattern == "xml")
		{
			if (!j.contains("filename")) { throw std::runtime_error("XML antenna pattern requires a 'filename'."); }
			ant = std::make_unique<XmlAntenna>(name, j.at("filename").get<std::string>());
		}
		else if (pattern == "file")
		{
			if (!j.contains("filename"))
			{
				throw std::runtime_error("H5 file antenna pattern requires a 'filename'.");
			}
			ant = std::make_unique<H5Antenna>(name, j.at("filename").get<std::string>());
		}
		else { throw std::runtime_error("Unsupported antenna pattern in from_json: " + pattern); }

		ant->setEfficiencyFactor(j.value("efficiency", 1.0));
	}
}

namespace radar
{

	void to_json(nlohmann::json& j, const Transmitter& t)
	{
		j = nlohmann::json{{"name", t.getName()},
		                   {"type", t.getPulsed() ? "pulsed" : "cw"},
		                   {"prf", t.getPrf()},
		                   {"pulse", t.getSignal() ? t.getSignal()->getName() : ""},
		                   {"antenna", t.getAntenna() ? t.getAntenna()->getName() : ""},
		                   {"timing", t.getTiming() ? t.getTiming()->getName() : ""}};
	}

	void to_json(nlohmann::json& j, const Receiver& r)
	{
		j = nlohmann::json{{"name", r.getName()},
		                   {"window_skip", r.getWindowSkip()},
		                   {"window_length", r.getWindowLength()},
		                   {"prf", r.getWindowPrf()},
		                   {"noise_temp", r.getNoiseTemperature()},
		                   {"antenna", r.getAntenna() ? r.getAntenna()->getName() : ""},
		                   {"timing", r.getTiming() ? r.getTiming()->getName() : ""},
		                   {"nodirect", r.checkFlag(Receiver::RecvFlag::FLAG_NODIRECT)},
		                   {"nopropagationloss", r.checkFlag(Receiver::RecvFlag::FLAG_NOPROPLOSS)}};
	}

	void to_json(nlohmann::json& j, const Target& t)
	{
		j["name"] = t.getName();
		nlohmann::json rcs_json;
		if (const auto* iso = dynamic_cast<const IsoTarget*>(&t))
		{
			rcs_json["type"] = "isotropic";
			rcs_json["value"] = iso->getConstRcs();
		}
		else if (const auto* file = dynamic_cast<const FileTarget*>(&t))
		{
			rcs_json["type"] = "file";
			rcs_json["filename"] = file->getFilename();
		}
		j["rcs"] = rcs_json;

		// Serialize the fluctuation model if it exists.
		if (const auto* model_base = t.getFluctuationModel())
		{
			nlohmann::json model_json;
			if (const auto* chi_model = dynamic_cast<const RcsChiSquare*>(model_base))
			{
				model_json["type"] = "chisquare";
				model_json["k"] = chi_model->getK();
			}
			else // Default to constant if it's not a recognized type (e.g., RcsConst)
			{
				model_json["type"] = "constant";
			}
			j["model"] = model_json;
		}
	}

	void to_json(nlohmann::json& j, const Platform& p)
	{
		j = {{"name", p.getName()}, {"motionpath", *p.getMotionPath()}};

		if (p.getRotationPath()->getType() == math::RotationPath::InterpType::INTERP_CONSTANT)
		{
			j["fixedrotation"] = *p.getRotationPath();
		}
		else { j["rotationpath"] = *p.getRotationPath(); }
	}

}

namespace params
{
	NLOHMANN_JSON_SERIALIZE_ENUM
	(CoordinateFrame,
	 {{CoordinateFrame::ENU, "ENU"},
	 {CoordinateFrame::UTM, "UTM"},
	 {CoordinateFrame::ECEF, "ECEF"}}
		)

	void to_json(nlohmann::json& j, const Parameters& p)
	{
		j = nlohmann::json{{"starttime", p.start},
		                   {"endtime", p.end},
		                   {"rate", p.rate},
		                   {"c", p.c},
		                   {"simSamplingRate", p.sim_sampling_rate},
		                   {"adc_bits", p.adc_bits},
		                   {"oversample", p.oversample_ratio}};

		if (p.random_seed.has_value()) { j["randomseed"] = p.random_seed.value(); }

		j["origin"] = {
			{"latitude", p.origin_latitude}, {"longitude", p.origin_longitude}, {"altitude", p.origin_altitude}};

		j["coordinatesystem"] = {{"frame", p.coordinate_frame}};
		if (p.coordinate_frame == CoordinateFrame::UTM)
		{
			j["coordinatesystem"]["zone"] = p.utm_zone;
			j["coordinatesystem"]["hemisphere"] = p.utm_north_hemisphere ? "N" : "S";
		}

		j["export"] = {{"xml", p.export_xml}, {"csv", p.export_csv}, {"binary", p.export_binary}};
	}

	void from_json(const nlohmann::json& j, Parameters& p)
	{
		p.start = j.at("starttime").get<RealType>();
		p.end = j.at("endtime").get<RealType>();
		p.rate = j.at("rate").get<RealType>();
		p.c = j.value("c", Parameters::DEFAULT_C);
		p.sim_sampling_rate = j.value("simSamplingRate", 1000.0);
		p.adc_bits = j.value("adc_bits", 0);
		p.oversample_ratio = j.value("oversample", 1);
		p.random_seed = j.value<std::optional<unsigned>>("randomseed", std::nullopt);

		const auto& origin = j.at("origin");
		p.origin_latitude = origin.at("latitude").get<double>();
		p.origin_longitude = origin.at("longitude").get<double>();
		p.origin_altitude = origin.at("altitude").get<double>();

		const auto& cs = j.at("coordinatesystem");
		p.coordinate_frame = cs.at("frame").get<CoordinateFrame>();
		if (p.coordinate_frame == CoordinateFrame::UTM)
		{
			p.utm_zone = cs.at("zone").get<int>();
			p.utm_north_hemisphere = (cs.at("hemisphere").get<std::string>() == "N");
		}

		const auto& exp = j.at("export");
		p.export_xml = exp.value("xml", false);
		p.export_csv = exp.value("csv", false);
		p.export_binary = exp.value("binary", true);
	}
}

namespace serial
{
	nlohmann::json world_to_json(const core::World& world)
	{
		nlohmann::json sim_json;

		sim_json["name"] = params::params.simulation_name;
		sim_json["parameters"] = params::params;

		sim_json["pulses"] = nlohmann::json::array();
		for (const auto& [name, pulse] : world.getPulses()) { sim_json["pulses"].push_back(*pulse); }

		sim_json["antennas"] = nlohmann::json::array();
		for (const auto& [name, antenna] : world.getAntennas()) { sim_json["antennas"].push_back(*antenna); }

		sim_json["timings"] = nlohmann::json::array();
		for (const auto& [name, timing] : world.getTimings()) { sim_json["timings"].push_back(*timing); }

		sim_json["platforms"] = nlohmann::json::array();
		for (const auto& p : world.getPlatforms())
		{
			nlohmann::json plat_json = *p;

			bool component_set = false;
			// Find the component associated with this platform. The logic must check for
			// monostatic pairs (a transmitter with an attached receiver) first to ensure
			// they are serialized as a single 'monostatic' component.
			for (const auto& t : world.getTransmitters())
			{
				if (t->getPlatform() == p.get())
				{
					if (t->getAttached() != nullptr)
					{
						// This is a monostatic pair. Serialize it as a single component.
						nlohmann::json monostatic_comp;
						monostatic_comp["name"] = t->getName();
						monostatic_comp["type"] = t->getPulsed() ? "pulsed" : "cw";
						monostatic_comp["pulse"] = t->getSignal() ? t->getSignal()->getName() : "";
						monostatic_comp["antenna"] = t->getAntenna() ? t->getAntenna()->getName() : "";
						monostatic_comp["timing"] = t->getTiming() ? t->getTiming()->getName() : "";
						monostatic_comp["prf"] = t->getPrf();

						if (const auto* recv = dynamic_cast<const radar::Receiver*>(t->getAttached()))
						{
							monostatic_comp["window_skip"] = recv->getWindowSkip();
							monostatic_comp["window_length"] = recv->getWindowLength();
							monostatic_comp["noise_temp"] = recv->getNoiseTemperature();
							monostatic_comp["nodirect"] = recv->checkFlag(radar::Receiver::RecvFlag::FLAG_NODIRECT);
							monostatic_comp["nopropagationloss"] =
								recv->checkFlag(radar::Receiver::RecvFlag::FLAG_NOPROPLOSS);
						}
						plat_json["component"] = {{"monostatic", monostatic_comp}};
					}
					else { plat_json["component"] = {{"transmitter", *t}}; }
					component_set = true;
					break; // A platform can only have one component.
				}
			}

			if (!component_set)
			{
				for (const auto& r : world.getReceivers())
				{
					if (r->getPlatform() == p.get())
					{
						// This must be a standalone receiver, as monostatic cases were handled above.
						if (r->getAttached() == nullptr)
						{
							plat_json["component"] = {{"receiver", *r}};
							component_set = true;
						}
						break;
					}
				}
			}

			if (!component_set)
			{
				for (const auto& target : world.getTargets())
				{
					if (target->getPlatform() == p.get())
					{
						plat_json["component"] = {{"target", *target}};
						component_set = true;
						break;
					}
				}
			}

			sim_json["platforms"].push_back(plat_json);
		}

		return {{"simulation", sim_json}};
	}

	void json_to_world(const nlohmann::json& j, core::World& world, std::mt19937& masterSeeder)
	{
		world.clear();

		const auto& sim = j.at("simulation");

		auto new_params = sim.at("parameters").get<params::Parameters>();

		if (sim.at("parameters").contains("randomseed"))
		{
			params::params.random_seed = new_params.random_seed;
			if (params::params.random_seed)
			{
				LOG(logging::Level::INFO, "Master seed updated from JSON to: {}", *params::params.random_seed);
				masterSeeder.seed(*params::params.random_seed);
			}
		}

		new_params.random_seed = params::params.random_seed;
		params::params = new_params;

		params::params.simulation_name = sim.value("name", "");

		// 2. Restore assets (Pulses, Antennas, Timings). These must be created first
		//    as platforms will reference them by name.
		if (sim.contains("pulses"))
		{
			auto pulses = sim.at("pulses").get<std::vector<std::unique_ptr<fers_signal::RadarSignal>>>();
			for (auto& pulse : pulses) { world.add(std::move(pulse)); }
		}

		if (sim.contains("antennas"))
		{
			auto antennas = sim.at("antennas").get<std::vector<std::unique_ptr<antenna::Antenna>>>();
			for (auto& antenna : antennas) { world.add(std::move(antenna)); }
		}

		if (sim.contains("timings"))
		{
			for (const auto& timing_json : sim.at("timings"))
			{
				auto name = timing_json.at("name").get<std::string>();
				auto timing_obj = std::make_unique<timing::PrototypeTiming>(name);
				from_json(timing_json, *timing_obj);
				world.add(std::move(timing_obj));
			}
		}

		// 3. Restore platforms and their components.
		if (sim.contains("platforms"))
		{
			for (const auto& plat_json : sim.at("platforms"))
			{
				auto name = plat_json.at("name").get<std::string>();
				auto plat = std::make_unique<radar::Platform>(name);

				// Paths
				if (plat_json.contains("motionpath"))
				{
					auto path = std::make_unique<math::Path>();
					from_json(plat_json.at("motionpath"), *path);
					plat->setMotionPath(std::move(path));
				}
				if (plat_json.contains("rotationpath"))
				{
					auto rot_path = std::make_unique<math::RotationPath>();
					from_json(plat_json.at("rotationpath"), *rot_path);
					plat->setRotationPath(std::move(rot_path));
				}
				else if (plat_json.contains("fixedrotation"))
				{
					auto rot_path = std::make_unique<math::RotationPath>();
					const auto& fixed_json = plat_json.at("fixedrotation");
					const RealType start_az_deg = fixed_json.at("startazimuth").get<RealType>();
					const RealType start_el_deg = fixed_json.at("startelevation").get<RealType>();
					const RealType rate_az_deg_s = fixed_json.at("azimuthrate").get<RealType>();
					const RealType rate_el_deg_s = fixed_json.at("elevationrate").get<RealType>();

					math::RotationCoord start, rate;
					start.azimuth = (90.0 - start_az_deg) * (PI / 180.0);
					start.elevation = start_el_deg * (PI / 180.0);
					rate.azimuth = -rate_az_deg_s * (PI / 180.0);
					rate.elevation = rate_el_deg_s * (PI / 180.0);
					rot_path->setConstantRate(start, rate);
					rot_path->finalize();
					plat->setRotationPath(std::move(rot_path));
				}

				// Component
				if (plat_json.contains("component"))
				{
					const auto& comp_json_outer = plat_json.at("component");
					if (comp_json_outer.contains("transmitter"))
					{
						const auto& comp_json = comp_json_outer.at("transmitter");
						auto trans = std::make_unique<radar::Transmitter>(
							plat.get(), comp_json.at("name").get<std::string>(),
							comp_json.at("type").get<std::string>() == "pulsed");
						trans->setPrf(comp_json.at("prf").get<RealType>());
						trans->setWave(world.findSignal(comp_json.at("pulse").get<std::string>()));
						trans->setAntenna(world.findAntenna(comp_json.at("antenna").get<std::string>()));
						const auto timing_name = comp_json.at("timing").get<std::string>();
						const auto timing = std::make_shared<timing::Timing>(timing_name, masterSeeder());
						timing->initializeModel(world.findTiming(timing_name));
						trans->setTiming(timing);
						world.add(std::move(trans));
					}
					else if (comp_json_outer.contains("receiver"))
					{
						const auto& comp_json = comp_json_outer.at("receiver");
						auto recv = std::make_unique<radar::Receiver>(plat.get(),
						                                              comp_json.at("name").get<std::string>(),
						                                              masterSeeder());
						recv->setWindowProperties(comp_json.at("window_length").get<RealType>(),
						                          comp_json.at("prf").get<RealType>(),
						                          comp_json.at("window_skip").get<RealType>());
						recv->setNoiseTemperature(comp_json.value("noise_temp", 0.0));
						recv->setAntenna(world.findAntenna(comp_json.at("antenna").get<std::string>()));
						if (comp_json.value("nodirect", false))
						{
							recv->setFlag(radar::Receiver::RecvFlag::FLAG_NODIRECT);
						}
						if (comp_json.value("nopropagationloss", false))
						{
							recv->setFlag(radar::Receiver::RecvFlag::FLAG_NOPROPLOSS);
						}
						const auto timing_name = comp_json.at("timing").get<std::string>();
						const auto timing = std::make_shared<timing::Timing>(timing_name, masterSeeder());
						timing->initializeModel(world.findTiming(timing_name));
						recv->setTiming(timing);
						world.add(std::move(recv));
					}
					else if (comp_json_outer.contains("target"))
					{
						const auto& comp_json = comp_json_outer.at("target");
						const auto& rcs_json = comp_json.at("rcs");
						const auto rcs_type = rcs_json.at("type").get<std::string>();
						std::unique_ptr<radar::Target> target_obj;
						if (rcs_type == "isotropic")
						{
							target_obj = radar::createIsoTarget(
								plat.get(), comp_json.at("name").get<std::string>(),
								rcs_json.at("value").get<RealType>(), masterSeeder());
						}
						else if (rcs_type == "file")
						{
							if (!rcs_json.contains("filename"))
							{
								throw std::runtime_error("File target requires an RCS filename.");
							}
							target_obj = radar::createFileTarget(
								plat.get(), comp_json.at("name").get<std::string>(),
								rcs_json.at("filename").get<std::string>(), masterSeeder());
						}
						else { throw std::runtime_error("Unsupported target RCS type: " + rcs_type); }
						world.add(std::move(target_obj));

						// After creating the target, check for and apply the fluctuation model.
						if (comp_json.contains("model"))
						{
							const auto& model_json = comp_json.at("model");
							const auto model_type = model_json.at("type").get<std::string>();
							if (model_type == "chisquare" || model_type == "gamma")
							{
								auto model = std::make_unique<radar::RcsChiSquare>(
									world.getTargets().back()->getRngEngine(),
									model_json.at("k").get<RealType>());
								world.getTargets().back()->setFluctuationModel(std::move(model));
							}
							// "constant" is the default, so no action is needed if that's the type.
						}
					}
					else if (comp_json_outer.contains("monostatic"))
					{
						const auto& comp_json = comp_json_outer.at("monostatic");

						// Transmitter part
						auto trans = std::make_unique<radar::Transmitter>(
							plat.get(), comp_json.at("name").get<std::string>(),
							comp_json.at("type").get<std::string>() == "pulsed");
						trans->setPrf(comp_json.at("prf").get<RealType>());
						trans->setWave(world.findSignal(comp_json.at("pulse").get<std::string>()));
						trans->setAntenna(world.findAntenna(comp_json.at("antenna").get<std::string>()));
						const auto timing_name = comp_json.at("timing").get<std::string>();
						const auto tx_timing = std::make_shared<timing::Timing>(timing_name, masterSeeder());
						tx_timing->initializeModel(world.findTiming(timing_name));
						trans->setTiming(tx_timing);

						// Receiver part
						auto recv = std::make_unique<radar::Receiver>(
							plat.get(), comp_json.at("name").get<std::string>(), masterSeeder());
						recv->setWindowProperties(comp_json.at("window_length").get<RealType>(),
						                          trans->getPrf(), comp_json.at("window_skip").get<RealType>());
						recv->setNoiseTemperature(comp_json.value("noise_temp", 0.0));
						recv->setAntenna(world.findAntenna(comp_json.at("antenna").get<std::string>()));
						if (comp_json.value("nodirect", false))
						{
							recv->setFlag(radar::Receiver::RecvFlag::FLAG_NODIRECT);
						}
						if (comp_json.value("nopropagationloss", false))
						{
							recv->setFlag(radar::Receiver::RecvFlag::FLAG_NOPROPLOSS);
						}
						const auto rx_timing = std::make_shared<timing::Timing>(timing_name, masterSeeder());
						rx_timing->initializeModel(world.findTiming(timing_name));
						recv->setTiming(rx_timing);

						// Link them and add to world
						trans->setAttached(recv.get());
						recv->setAttached(trans.get());
						world.add(std::move(trans));
						world.add(std::move(recv));
					}
				}

				world.add(std::move(plat));
			}
		}
	}
}
