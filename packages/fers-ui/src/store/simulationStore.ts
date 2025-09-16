import {create} from 'zustand';
import type {AnySimulationElement} from '@/types/simulation';

// Mock data now uses the union type, which allows for type-specific properties.
const MOCK_ELEMENTS: AnySimulationElement[] = [
    {id: 'p1', name: 'Monostatic Platform', type: 'Platform', parentId: null, position: {x: 0, y: 0, z: 0}},
    {id: 'a1', name: 'Main Antenna', type: 'Antenna', parentId: 'p1'},
    {id: 't1', name: 'Target A', type: 'Target', parentId: null, rcs: 10},
    {id: 't2', name: 'Target B', type: 'Target', parentId: null, rcs: 5},
];


interface SimulationState {
    elements: AnySimulationElement[];
    selectedElementId: string | null;
    actions: {
        selectElement: (id: string | null) => void;
        getElementById: (id: string) => AnySimulationElement | undefined;
    };
}

export const useSimulationStore = create<SimulationState>((set, get) => ({
    elements: MOCK_ELEMENTS,
    selectedElementId: null,
    actions: {
        selectElement: (id) => set({selectedElementId: id}),
        getElementById: (id) => get().elements.find(el => el.id === id),
    },
}));

// Export actions separately for convenient usage in components
export const useSimulationActions = () => useSimulationStore((state) => state.actions);
