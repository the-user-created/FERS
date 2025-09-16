import {
    Paper,
    Typography,
    List,
    ListItem,
    ListItemButton,
    ListItemText,
    Divider,
} from '@mui/material';
import {
    useSimulationStore,
    useSimulationActions,
} from '@/store/simulationStore';

/**
 * The RightSidebar component serves as the WorldView Hierarchy.
 * It displays a tree view of all elements in the simulation.
 */
export function RightSidebar() {
    const elements = useSimulationStore((state) => state.elements);
    const selectedElementId = useSimulationStore(
        (state) => state.selectedElementId
    );
    const { selectElement } = useSimulationActions();

    return (
        <Paper
            elevation={2}
            sx={{
                p: 2,
                height: '100%',
                overflowY: 'auto',
            }}
        >
            <Typography variant="h6" gutterBottom>
                Scene Hierarchy
            </Typography>
            <Divider sx={{ mb: 1 }} />
            <List dense>
                {elements.map((element) => (
                    <ListItem key={element.id} disablePadding>
                        <ListItemButton
                            selected={selectedElementId === element.id}
                            onClick={() => selectElement(element.id)}
                        >
                            <ListItemText
                                primary={element.name}
                                secondary={element.type}
                            />
                        </ListItemButton>
                    </ListItem>
                ))}
            </List>
        </Paper>
    );
}
