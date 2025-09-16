import { useRef } from 'react';
import { useFrame } from '@react-three/fiber';
import { OrbitControls } from '@react-three/drei';
import { Mesh } from 'three';

function SpinningBox() {
    const meshRef = useRef<Mesh>(null!);

    useFrame((_state, delta) => {
        if (meshRef.current) {
            meshRef.current.rotation.x += delta * 0.2;
            meshRef.current.rotation.y += delta * 0.2;
        }
    });

    return (
        <mesh ref={meshRef} position={[0, 1.5, 0]}>
            <boxGeometry args={[2, 2, 2]} />
            <meshStandardMaterial color={'#f48fb1'} />
        </mesh>
    );
}

/**
 * WorldView represents the 3D scene where simulation elements are visualized.
 */
export default function WorldView() {
    return (
        <>
            {/* Controls */}
            <OrbitControls makeDefault />

            {/* Lights */}
            <ambientLight intensity={Math.PI / 2} />
            <spotLight
                position={[10, 15, 10]}
                angle={0.3}
                penumbra={1}
                decay={0}
                intensity={Math.PI}
            />
            <pointLight
                position={[-10, -15, -10]}
                decay={0}
                intensity={Math.PI}
            />

            {/* Scenery */}
            <gridHelper args={[100, 100]} />

            {/* Objects */}
            <SpinningBox />
        </>
    );
}
