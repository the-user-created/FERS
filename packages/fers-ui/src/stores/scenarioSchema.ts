import { z } from 'zod';

// Base numeric type for zod schemas - handles empty strings from forms
const nullableNumber = z.preprocess(
    (val) => (val === '' ? null : val),
    z.number().nullable()
);

// --- SCHEMA DEFINITIONS ---

export const GlobalParametersSchema = z.object({
    id: z.literal('global-parameters'),
    type: z.literal('GlobalParameters'),
    simulation_name: z.string().min(1, 'Simulation name cannot be empty.'),
    start: z.number(),
    end: z.number(),
    rate: z.number().positive('Rate must be positive.'),
    simSamplingRate: nullableNumber.refine((val) => val === null || val > 0, {
        message: 'Sim Sampling Rate must be positive if specified.',
    }),
    c: z.number().positive('Speed of light must be positive.'),
    random_seed: nullableNumber.pipe(z.number().int().nullable()),
    adc_bits: z.number().int().min(0, 'ADC bits cannot be negative.'),
    oversample_ratio: z
        .number()
        .int()
        .min(1, 'Oversample ratio must be at least 1.'),
    origin: z.object({
        latitude: z.number().min(-90).max(90),
        longitude: z.number().min(-180).max(180),
        altitude: z.number(),
    }),
    coordinateSystem: z
        .object({
            frame: z.enum(['ENU', 'UTM', 'ECEF']),
            zone: z.number().int().optional(),
            hemisphere: z.enum(['N', 'S']).optional(),
        })
        .refine(
            (data) => {
                if (data.frame === 'UTM') {
                    return (
                        data.zone !== undefined && data.hemisphere !== undefined
                    );
                }
                return true;
            },
            { message: 'UTM frame requires a zone and hemisphere.' }
        ),
    export: z.object({
        xml: z.boolean(),
        csv: z.boolean(),
        binary: z.boolean(),
    }),
});

export const PulseSchema = z
    .object({
        id: z.string().uuid(),
        type: z.literal('Pulse'),
        name: z.string().min(1, 'Pulse name cannot be empty.'),
        pulseType: z.enum(['file', 'cw']),
        power: z.number().min(0, 'Power cannot be negative.'),
        carrier: z.number().positive('Carrier frequency must be positive.'),
        filename: z.string().optional(),
    })
    .refine(
        (data) => {
            if (data.pulseType === 'file') {
                return data.filename !== undefined && data.filename.length > 0;
            }
            return true;
        },
        {
            message: 'A filename is required for pulse type "file".',
            path: ['filename'],
        }
    );

export const NoiseEntrySchema = z.object({
    id: z.string().uuid(),
    alpha: z.number(),
    weight: z.number(),
});

export const TimingSchema = z.object({
    id: z.string().uuid(),
    type: z.literal('Timing'),
    name: z.string().min(1, 'Timing name cannot be empty.'),
    frequency: z.number().positive('Frequency must be positive.'),
    freqOffset: nullableNumber,
    randomFreqOffsetStdev: nullableNumber.pipe(z.number().min(0).nullable()),
    phaseOffset: nullableNumber,
    randomPhaseOffsetStdev: nullableNumber.pipe(z.number().min(0).nullable()),
    noiseEntries: z.array(NoiseEntrySchema),
});

const BaseAntennaSchema = z.object({
    id: z.string().uuid(),
    type: z.literal('Antenna'),
    name: z.string().min(1, 'Antenna name cannot be empty.'),
    efficiency: nullableNumber.pipe(z.number().min(0).max(1).nullable()),
});

export const AntennaSchema = z.discriminatedUnion('pattern', [
    BaseAntennaSchema.extend({ pattern: z.literal('isotropic') }),
    BaseAntennaSchema.extend({
        pattern: z.literal('sinc'),
        alpha: nullableNumber.pipe(z.number().nullable()),
        beta: nullableNumber.pipe(z.number().nullable()),
        gamma: nullableNumber.pipe(z.number().nullable()),
    }),
    BaseAntennaSchema.extend({
        pattern: z.literal('gaussian'),
        azscale: nullableNumber.pipe(z.number().nullable()),
        elscale: nullableNumber.pipe(z.number().nullable()),
    }),
    BaseAntennaSchema.extend({
        pattern: z.literal('squarehorn'),
        diameter: nullableNumber.pipe(z.number().positive().nullable()),
    }),
    BaseAntennaSchema.extend({
        pattern: z.literal('parabolic'),
        diameter: nullableNumber.pipe(z.number().positive().nullable()),
    }),
    BaseAntennaSchema.extend({
        pattern: z.literal('xml'),
        filename: z
            .string()
            .min(1, 'Filename is required for XML pattern.')
            .optional(),
    }),
    BaseAntennaSchema.extend({
        pattern: z.literal('file'),
        filename: z
            .string()
            .min(1, 'Filename is required for file pattern.')
            .optional(),
    }),
]);

export const PositionWaypointSchema = z.object({
    id: z.string().uuid(),
    x: z.number(),
    y: z.number(),
    altitude: z.number(),
    time: z.number().min(0, 'Time cannot be negative.'),
});

export const MotionPathSchema = z.object({
    interpolation: z.enum(['static', 'linear', 'cubic']),
    waypoints: z
        .array(PositionWaypointSchema)
        .min(1, 'At least one waypoint is required.'),
});

export const FixedRotationSchema = z.object({
    type: z.literal('fixed'),
    startAzimuth: z.number(),
    startElevation: z.number(),
    azimuthRate: z.number(),
    elevationRate: z.number(),
});

export const RotationWaypointSchema = z.object({
    id: z.string().uuid(),
    azimuth: z.number(),
    elevation: z.number(),
    time: z.number().min(0, 'Time cannot be negative.'),
});

export const RotationPathSchema = z.object({
    type: z.literal('path'),
    interpolation: z.enum(['static', 'linear', 'cubic']),
    waypoints: z
        .array(RotationWaypointSchema)
        .min(1, 'At least one waypoint is required.'),
});

const NoComponentSchema = z.object({ type: z.literal('none') });
const MonostaticComponentSchema = z.object({
    type: z.literal('monostatic'),
    name: z.string().min(1),
    radarType: z.enum(['pulsed', 'cw']),
    window_skip: z.number().min(0),
    window_length: z.number().min(0),
    prf: z.number().positive(),
    antennaId: z.string().uuid().nullable(),
    pulseId: z.string().uuid().nullable(),
    timingId: z.string().uuid().nullable(),
    noiseTemperature: nullableNumber.pipe(z.number().min(0).nullable()),
    noDirectPaths: z.boolean(),
    noPropagationLoss: z.boolean(),
});
const TransmitterComponentSchema = z.object({
    type: z.literal('transmitter'),
    name: z.string().min(1),
    radarType: z.enum(['pulsed', 'cw']),
    prf: z.number().positive(),
    antennaId: z.string().uuid().nullable(),
    pulseId: z.string().uuid().nullable(),
    timingId: z.string().uuid().nullable(),
});
const ReceiverComponentSchema = z.object({
    type: z.literal('receiver'),
    name: z.string().min(1),
    window_skip: z.number().min(0),
    window_length: z.number().min(0),
    prf: z.number().positive(),
    antennaId: z.string().uuid().nullable(),
    timingId: z.string().uuid().nullable(),
    noiseTemperature: nullableNumber.pipe(z.number().min(0).nullable()),
    noDirectPaths: z.boolean(),
    noPropagationLoss: z.boolean(),
});
const TargetComponentSchema = z.object({
    type: z.literal('target'),
    name: z.string().min(1),
    rcs_type: z.enum(['isotropic', 'file']),
    rcs_value: z.number().optional(),
    rcs_filename: z.string().optional(),
    rcs_model: z.enum(['constant', 'chisquare', 'gamma']),
    rcs_k: z.number().optional(),
});

export const PlatformComponentSchema = z.discriminatedUnion('type', [
    NoComponentSchema,
    MonostaticComponentSchema,
    TransmitterComponentSchema,
    ReceiverComponentSchema,
    TargetComponentSchema,
]);

export const PlatformSchema = z.object({
    id: z.string().uuid(),
    type: z.literal('Platform'),
    name: z.string().min(1, 'Platform name cannot be empty.'),
    motionPath: MotionPathSchema,
    rotation: z.union([FixedRotationSchema, RotationPathSchema]),
    component: PlatformComponentSchema,
});

export const ScenarioDataSchema = z.object({
    globalParameters: GlobalParametersSchema,
    pulses: z.array(PulseSchema),
    timings: z.array(TimingSchema),
    antennas: z.array(AntennaSchema),
    platforms: z.array(PlatformSchema),
});
