// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

#include "serial/json_serializer.h"

#include <map>
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
#include "signal/radar_signal.h"
#include "timing/prototype_timing.h"
#include "timing/timing.h"

namespace math
{

	void to_json(nlohmann::json& j, const Vec3& v) { j = {{"x", v.x}, {"y", v.y}, {"z", v.z}}; }

	void to_json(nlohmann::json& j, const Coord& c)
	{
		j = {{"time", c.t}, {"x", c.pos.x}, {"y", c.pos.y}, {"altitude", c.pos.z}};
	}

	void to_json(nlohmann::json& j, const RotationCoord& rc)
	{
		// Convert radians back to compass degrees for UI consistency with XML
		const RealType az_deg = std::fmod(90.0 - (rc.azimuth * 180.0 / PI) + 360.0, 360.0);
		const RealType el_deg = rc.elevation * 180.0 / PI;
		j = {{"time", rc.t}, {"azimuth", az_deg}, {"elevation", el_deg}};
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

	NLOHMANN_JSON_SERIALIZE_ENUM
	(RotationPath::InterpType,
	 {{RotationPath::InterpType::INTERP_STATIC, "static"},
	 {RotationPath::InterpType::INTERP_CONSTANT, "constant"},
	 {RotationPath::InterpType::INTERP_LINEAR, "linear"},
	 {RotationPath::InterpType::INTERP_CUBIC, "cubic"}}
		)

	void to_json(nlohmann::json& j, const RotationPath& p)
	{
		j["interpolation"] = p.getType();
		if (p.getType() == RotationPath::InterpType::INTERP_CONSTANT)
		{
			// This corresponds to <fixedrotation>
			const RealType start_az_deg = std::fmod(90.0 - (p.getStart().azimuth * 180.0 / PI) + 360.0, 360.0);
			const RealType start_el_deg = p.getStart().elevation * 180.0 / PI;
			const RealType rate_az_deg_s = -p.getRate().azimuth * 180.0 / PI;
			const RealType rate_el_deg_s = p.getRate().elevation * 180.0 / PI;
			j["startazimuth"] = start_az_deg;
			j["startelevation"] = start_el_deg;
			j["azimuthrate"] = rate_az_deg_s;
			j["elevationrate"] = rate_el_deg_s;
		}
		else { j["rotationwaypoints"] = p.getCoords(); }
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
}

namespace fers_signal
{
	void to_json(nlohmann::json& j, const RadarSignal& rs)
	{
		j = nlohmann::json{{"name", rs.getName()}, {"power", rs.getPower()}, {"carrier", rs.getCarrier()}};
		if (dynamic_cast<const CwSignal*>(rs.getSignal())) { j["type"] = "continuous"; }
		else { j["type"] = "file"; }
	}
}

namespace antenna
{
	void to_json(nlohmann::json& j, const Antenna& a)
	{
		j = {{"name", a.getName()}, {"efficiency", a.getEfficiencyFactor()}};

		if (const auto* iso = dynamic_cast<const Isotropic*>(&a)) { j["pattern"] = "isotropic"; }
		else if (const auto* sinc = dynamic_cast<const Sinc*>(&a))
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
		else if (const auto* xml = dynamic_cast<const XmlAntenna*>(&a)) { j["pattern"] = "xml"; }
		else
			if (const auto* h5 = dynamic_cast<const H5Antenna*>(&a)) { j["pattern"] = "file"; }
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
		else
			if (const auto* file = dynamic_cast<const FileTarget*>(&t)) { rcs_json["type"] = "file"; }
		j["rcs"] = rcs_json;
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

		std::map<const radar::Platform*, nlohmann::json> platform_map;
		for (const auto& p : world.getPlatforms()) { platform_map[p.get()] = *p; }

		for (const auto& t : world.getTransmitters())
		{
			if (platform_map.count(t->getPlatform()))
			{
				if (t->getAttached() != nullptr)
				{
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
					platform_map.at(t->getPlatform())["component"] = {{"monostatic", monostatic_comp}};
				}
				else { platform_map.at(t->getPlatform())["component"] = {{"transmitter", *t}}; }
			}
		}

		for (const auto& r : world.getReceivers())
		{
			if (platform_map.count(r->getPlatform()) && r->getAttached() == nullptr)
			{
				platform_map.at(r->getPlatform())["component"] = {{"receiver", *r}};
			}
		}

		for (const auto& t : world.getTargets())
		{
			if (platform_map.count(t->getPlatform()))
			{
				platform_map.at(t->getPlatform())["component"] = {{"target", *t}};
			}
		}

		sim_json["platforms"] = nlohmann::json::array();
		for (const auto& pair : platform_map) { sim_json["platforms"].push_back(pair.second); }

		return {{"simulation", sim_json}};
	}
}
