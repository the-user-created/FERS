// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

/**
 * A utility function for exhaustive checks in switch statements.
 *
 * If a switch statement should handle all possible cases of a union type,
 * you can use this function in the `default` case. TypeScript will ensure
 * that the type of the variable passed to this function is `never`, meaning
 * all other cases have been exhausted. If you add a new type to the union
 * and forget to handle it in the switch, the compiler will raise an error
 * at the call site of this function.
 *
 * @param x - The variable that should be of type `never`.
 * @returns This function never returns; it throws an error.
 */
export function assertNever(x: never): never {
    throw new Error(`Unexpected object: ${JSON.stringify(x)}`);
}
