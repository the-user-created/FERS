// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2025-present FERS Contributors (see AUTHORS.md).

import { Box, BoxProps } from '@mui/material';
import React, { useState, useRef, useCallback, useEffect } from 'react';

type ResizablePanelProps = {
    children: React.ReactNode;
    initialSize: number;
    minSize?: number;
    maxSize?: number;
    direction: 'horizontal' | 'vertical';
    side: 'left' | 'right' | 'top' | 'bottom';
} & BoxProps;

const ResizablePanel: React.FC<ResizablePanelProps> = ({
    children,
    initialSize,
    minSize = 50,
    maxSize = 800,
    direction,
    side,
    ...boxProps
}) => {
    const [size, setSize] = useState(initialSize);
    const isResizing = useRef(false);

    const handleMouseMove = useCallback(
        (e: MouseEvent) => {
            if (!isResizing.current) return;
            let newSize;
            if (direction === 'horizontal') {
                newSize =
                    side === 'left' ? e.clientX : window.innerWidth - e.clientX;
            } else {
                // vertical
                newSize =
                    side === 'top' ? e.clientY : window.innerHeight - e.clientY;
            }

            if (newSize < minSize) newSize = minSize;
            if (newSize > maxSize) newSize = maxSize;
            setSize(newSize);
        },
        [direction, minSize, maxSize, side]
    );

    const handleMouseUp = useCallback(() => {
        isResizing.current = false;
        document.removeEventListener('mousemove', handleMouseMove);
        document.removeEventListener('mouseup', handleMouseUp);
        document.body.style.cursor = 'default';
        document.body.style.userSelect = 'auto';
    }, [handleMouseMove]);

    const handleMouseDown = (e: React.MouseEvent) => {
        e.preventDefault();
        isResizing.current = true;
        document.addEventListener('mousemove', handleMouseMove);
        document.addEventListener('mouseup', handleMouseUp);
        document.body.style.cursor =
            direction === 'horizontal' ? 'col-resize' : 'row-resize';
        document.body.style.userSelect = 'none';
    };

    // Cleanup listeners on unmount
    useEffect(() => {
        return () => {
            document.removeEventListener('mousemove', handleMouseMove);
            document.removeEventListener('mouseup', handleMouseUp);
        };
    }, [handleMouseMove, handleMouseUp]);

    const resizerStyles = {
        cursor: direction === 'horizontal' ? 'col-resize' : 'row-resize',
        width: direction === 'horizontal' ? '5px' : '100%',
        height: direction === 'horizontal' ? '100%' : '5px',
        position: 'absolute',
        ...(direction === 'horizontal'
            ? { [side === 'left' ? 'right' : 'left']: -2.5 }
            : {}),
        ...(direction === 'vertical'
            ? { [side === 'top' ? 'bottom' : 'top']: -2.5 }
            : {}),
        zIndex: 10,
        '&:hover': {
            backgroundColor: 'action.hover',
        },
    };

    const panelStyles = {
        [direction === 'horizontal' ? 'width' : 'height']: `${size}px`,
        position: 'relative',
        display: 'flex',
        flexDirection: 'column',
        flexShrink: 0,
    };

    return (
        <Box sx={panelStyles} {...boxProps}>
            <Box onMouseDown={handleMouseDown} sx={resizerStyles} />
            <Box sx={{ flex: 1, overflow: 'auto', p: 1 }}>{children}</Box>
        </Box>
    );
};

export default ResizablePanel;
