import { Box } from '@mui/material';
import { Canvas } from '@react-three/fiber';
import WorldView from '@/components/WorldView';

/**
 * MainView contains the 3D rendering canvas.
 */
export function MainView() {
    return (
        <Box
            sx={{
                flex: 1,
                height: '100%',
                backgroundColor: 'black',
            }}
        >
            <Canvas camera={{ position: [10, 10, 10], fov: 25 }}>
                <WorldView />
            </Canvas>
        </Box>
    );
}
