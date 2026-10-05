import {ChangeEvent, useEffect, useRef, useState} from "react";
import useMousePosition from "./hooks/useMousePosition";
import * as React from "react";
import 'toolcool-color-picker';
import ColorPicker from "./ColorPicker";
import {Hold} from "./hooks/useBoardApi";

export interface Pixel { holdnum: number; r: number; g: number; b: number; }

interface DrawCanvasProps {
    // PixelMapping[y][x] => holdnum (or -1) - fetched from the gateway
    grid: number[][];
    // holds already on the board (drawn once when the app loads)
    initialHolds: Hold[];
    onPublish: (pixels: Pixel[]) => Promise<{ sent: number; connected: boolean; state: string }>;
    onClearBoard: () => Promise<void>;
}

const CELL = 20;
const RADIUS = [5, 5, 5, 5] as unknown as number;

let ActivePixels: Array<Pixel> = [];   // active pixels (addPixel)

export default function DrawCanvas({ grid, initialHolds, onPublish, onClearBoard }: DrawCanvasProps)
{
    const canvasRef = useRef<HTMLCanvasElement | null>(null);
    const previewCanvasRef = useRef<HTMLCanvasElement | null>(null);
    const initializedRef = useRef(false);
    const [coords, handleCoords] = useMousePosition(true);
    const [drawingColor, setDrawingColor] = React.useState("rgb(255,0,0)"); // Default color is #ff0000
    const [drawing, setDrawing] = useState<boolean>(false);
    const [message, setMessage] = useState<string>("");
    const [gridCoords, setGridCoords] = useState<{ x: number; y: number }>({ x: 0, y: 0 });

    const cols = grid.length > 0 ? grid[0].length : 27;
    const rows = grid.length;

    // resolve a hold number or -1 if there is no hold at this position
    const getHoldNum = (x: number, y: number): number => {
        if (!grid[y]) return -1;
        return grid[y][x] ?? -1;
    };

    // Draw empty grid (outline of every hold position)
    const initGrid = () => {
        const ctx = canvasRef?.current?.getContext("2d");
        if (ctx) {
            for (let x = 0; x < cols; x++) {
                for (let y = 0; y < rows; y++) {
                    if (getHoldNum(x, y) !== -1) {
                        ctx.beginPath();
                        // @ts-ignore
                        ctx.roundRect(x * CELL + 2, y * CELL + 2, 16, 16, RADIUS);
                        ctx.lineWidth = 1;
                        ctx.stroke();
                    }
                }
            }
        }
    };

    const fillCell = (ctx: CanvasRenderingContext2D, x: number, y: number, color: string) => {
        ctx.beginPath();
        // @ts-ignore
        ctx.roundRect(x * CELL + 2, y * CELL + 2, 16, 16, RADIUS);
        ctx.stroke();
        ctx.fillStyle = color;
        ctx.fill();
    };

    const clearCanvas = () => {
        const ctx = canvasRef.current?.getContext("2d");
        ctx?.clearRect(0, 0, cols * CELL, rows * CELL);
        if (ctx) initGrid();
    };

    // draw grid + existing board state once the mapping is available
    useEffect(() => {
        if (grid.length === 0 || initializedRef.current) return;
        initializedRef.current = true;
        initGrid();
        const ctx = canvasRef.current?.getContext("2d");
        if (ctx) {
            for (const h of initialHolds) {
                fillCell(ctx, h.x, h.y, `rgb(${h.r},${h.g},${h.b})`);
            }
        }
        // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [grid, initialHolds]);

    async function toBase64(file: any) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.readAsDataURL(file);
            reader.onload = () => resolve(reader.result);
            reader.onerror = error => reject(error);
        });
    }

    function timeout(delay: number) {
        return new Promise(res => setTimeout(res, delay));
    }

    const savePixels = () => {
        if (canvasRef.current) {
            let canvasUrl = canvasRef.current?.toDataURL();
            const createEl = document.createElement('a');
            createEl.href = canvasUrl;
            createEl.download = "saved-canvas";
            createEl.click();
            createEl.remove();
        }
    }

    const testPixel = (holdnum: number): boolean => {
        for (let i = 0; i < ActivePixels.length; i++) {
            if (ActivePixels[i].holdnum === holdnum)
                return true;
        }
        return false;
    }

    const addPixel = (x: number, y: number, r: number, g: number, b: number) => {
        let holdnum = getHoldNum(x, y);
        if ((holdnum !== -1) && !testPixel(holdnum)) {
            ActivePixels.push({ holdnum, r, g, b });
        }
    };

    const deletePixel = (x: number, y: number) => {
        let holdnum = getHoldNum(x, y);
        if (holdnum !== -1) {
            for (let i = 0; i < ActivePixels.length; i++) {
                if (ActivePixels[i].holdnum === holdnum) {
                    ActivePixels.splice(i, 1);
                    break;
                }
            }
        }
    };

    const publish = async () => {
        try {
            const res = await onPublish(ActivePixels);
            setMessage(res.connected
                ? `Sent ${res.sent} holds to the board`
                : `Queued ${res.sent} holds (gateway not connected to board yet - waiting...)`);
        } catch (e: any) {
            setMessage("Publish failed: " + (e?.message ?? String(e)));
        }
    };

    const clearBoard = async () => {
        ActivePixels = [];
        clearCanvas();
        try {
            await onClearBoard();
            setMessage("Board cleared");
        } catch (e: any) {
            setMessage("Clear failed: " + (e?.message ?? String(e)));
        }
    };

    const handleLoadImage = (event: ChangeEvent<HTMLInputElement>) => {
        const { files } = event.target;
        if (files && files[0]) {
            if (canvasRef.current) {
                const ctx = canvasRef.current?.getContext("2d");
                ctx?.clearRect(0, 0, cols * CELL, rows * CELL);
                const previewCtx = previewCanvasRef?.current?.getContext("2d");
                previewCtx?.clearRect(0, 0, cols, rows);
                ActivePixels = [];
                if (ctx) initGrid();
            }

            toBase64(files[0]).then(value => {
                let img = new Image();
                if (typeof value === "string") {
                    img.src = value;
                }
                img.onload = async () => {
                    ActivePixels = [];
                    await timeout(500);
                    const previewCtx = previewCanvasRef?.current?.getContext("2d");
                    const ctx = canvasRef?.current?.getContext("2d");
                    if (previewCtx && ctx) {
                        previewCtx.drawImage(img, 0, 0, cols, rows);
                        for (let x = 0; x < cols; x++) {
                            for (let y = 0; y < rows; y++) {
                                let data = previewCtx.getImageData(x, y, 1, 1).data;
                                if ((data[0] !== 0xff || data[1] !== 0xff || data[2] !== 0xff) &&
                                    (data[0] !== 0x0 || data[1] !== 0x0 || data[2] !== 0x0) &&
                                    (getHoldNum(x, y) !== -1)) {
                                    addPixel(x, y, data[0], data[1], data[2]);
                                    fillCell(ctx, x, y, `rgb(${data[0]},${data[1]},${data[2]})`);
                                }
                            }
                        }
                    }
                }
            })
        }
    }

    return (
        <>
            <h1>Etch a KilterBoard <small className="text-muted">v2.0 (REST)</small></h1>
            <canvas
                ref={canvasRef}
                width={cols * CELL}
                height={rows * CELL}
                style={{ border: "2px solid black", touchAction: "none" }}

                // remove pixel (right click)
                onContextMenu={(e) => {
                    e.preventDefault();
                    if (canvasRef.current) {
                        const ctx = canvasRef.current?.getContext("2d");
                        if (ctx) {
                            let x = Math.round((coords.x - 10) / CELL);
                            let y = Math.round((coords.y - 10) / CELL);
                            if (x > cols - 1) x = cols - 1;
                            if (y > rows - 1) y = rows - 1;

                            if (!isNaN(x) && !isNaN(y) && getHoldNum(x, y) !== -1) {
                                ctx.clearRect(x * CELL - 1, y * CELL - 1, CELL + 2, CELL + 2);
                                deletePixel(x, y);
                            }
                        }
                    }
                }}
                onPointerDown={(e) => {
                    e.preventDefault();
                    setDrawing(true);
                }}
                onPointerUp={(e) => {
                    e.preventDefault();
                    setDrawing(false);
                }}
                onPointerMove={(e) => {
                    handleCoords((e as unknown) as any);
                    if (canvasRef.current && drawing) {
                        const ctx = canvasRef.current?.getContext("2d");
                        if (ctx) {
                            let x = Math.round((coords.x - 10) / CELL);
                            let y = Math.round((coords.y - 10) / CELL);
                            if (x > cols - 1) x = cols - 1;
                            if (y > rows - 1) y = rows - 1;

                            setGridCoords({ x, y });

                            if (!isNaN(x) && !isNaN(y) && getHoldNum(x, y) !== -1) {
                                fillCell(ctx, x, y, drawingColor);
                                let rgb = drawingColor.replace(/[^\d,]/g, '').split(',');
                                addPixel(x, y, Number(rgb[0]), Number(rgb[1]), Number(rgb[2]));
                            }
                        }
                    }
                }}
            ></canvas>
            <p></p>
            <ColorPicker onChangeColor={setDrawingColor} />
            <span> </span>
            <button type="button" className="btn btn-warning" onClick={() => {
                ActivePixels = [];
                clearCanvas();
            }}>
                CLEAR
            </button>
            <span> </span>
            <button type="button" className="btn btn-success" onClick={publish}>
                PUBLISH
            </button>
            <span> </span>
            <button type="button" className="btn btn-danger" onClick={clearBoard}>
                OFF
            </button>
            <span> </span>
            <button type="button" className="btn btn-success" onClick={savePixels}>
                SAVE
            </button>
            <span> </span>
            <input type="file" className="btn btn-outline-primary" name="image" placeholder='Image' accept="image/*" onChange={e => handleLoadImage(e)} />
            <canvas
                ref={previewCanvasRef}
                width={cols}
                height={rows}
                style={{ visibility: 'hidden' }}
            />
            <p></p>
            {message && <p className="alert alert-info py-1 px-2">{message}</p>}
            <p><>Draw: {drawing ? "yes" : "no"}</></p>
            <p><>X: {coords.x}  Y: {coords.y}</></p>
            <p><>Col: {gridCoords.x}  Row: {gridCoords.y}</></p>
        </>
    );
}