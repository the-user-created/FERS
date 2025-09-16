import {Box, Paper, Typography, Divider} from '@mui/material';
import {useSimulationStore} from '@/store/simulationStore';

/**
 * The LeftSidebar component serves as the Property Inspector.
 * It displays and allows editing of parameters for the currently
 * selected simulation element.
 */
export function LeftSidebar() {
    const selectedElementId = useSimulationStore((state) => state.selectedElementId);
    const getElementById = useSimulationStore((state) => state.actions.getElementById);

    const selectedElement = selectedElementId ? getElementById(selectedElementId) : null;

    return (
        <Paper
            elevation={2}
            sx={{
                p: 2,
                height: '100%',
                overflowY: 'auto',
                display: 'flex',
                flexDirection: 'column',
            }}
        >
            <Typography variant="h6" gutterBottom>
                Property Inspector
            </Typography>
            <Divider sx={{mb: 2}}/>
            <Box sx={{flex: 1}}>
                {selectedElement ? (
                    <Box>
                        <Typography variant="subtitle1"><b>Name:</b> {selectedElement.name}</Typography>
                        <Typography variant="body2"><b>ID:</b> {selectedElement.id}</Typography>
                        <Typography variant="body2"><b>Type:</b> {selectedElement.type}</Typography>
                        <pre style={{whiteSpace: 'pre-wrap', wordBreak: 'break-all', fontSize: '0.8rem'}}>
                            {JSON.stringify(selectedElement, null, 2)}
                        </pre>
                    </Box>
                ) : (
                    <Typography variant="body2" color="text.secondary">
                        Select an element from the Scene Hierarchy to see its properties.
                    </Typography>
                )}
            </Box>
        </Paper>
    );
}