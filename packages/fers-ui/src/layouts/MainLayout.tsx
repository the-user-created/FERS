import {Box} from '@mui/material';
import {LeftSidebar} from '@/views/LeftSidebar';
import {RightSidebar} from '@/views/RightSidebar';
import {MainView} from '@/views/MainView';

const LEFT_SIDEBAR_WIDTH = 300;
const RIGHT_SIDEBAR_WIDTH = 280;

/**
 * Defines the main three-panel structure of the application:
 * - Left Sidebar (Property Inspector)
 * - Main Content (3D World View)
 * - Right Sidebar (WorldView Hierarchy)
 */
export function MainLayout() {
    return (
        <Box sx={{display: 'flex', height: '100vh', width: '100vw', overflow: 'hidden'}}>
            {/* Left Sidebar: Property Inspector */}
            <Box
                component="aside"
                sx={{
                    width: LEFT_SIDEBAR_WIDTH,
                    flexShrink: 0,
                    height: '100%',
                }}
            >
                <LeftSidebar/>
            </Box>

            {/* Main Content: 3D Viewport */}
            <Box
                component="main"
                sx={{
                    flexGrow: 1,
                    height: '100%',
                    minWidth: 0, // Prevents flexbox from overflowing
                }}
            >
                <MainView/>
            </Box>

            {/* Right Sidebar: WorldView Hierarchy */}
            <Box
                component="aside"
                sx={{
                    width: RIGHT_SIDEBAR_WIDTH,
                    flexShrink: 0,
                    height: '100%',
                }}
            >
                <RightSidebar/>
            </Box>
        </Box>
    );
}
