import { useCallback, useEffect, useState } from "react";

// ---------------------------------------------------------------------------
// REST client for the MonkeyBoard Gateway (mb-esp32)
// Served from the same origin (ESP32 LittleFS), so all calls are relative.
// ---------------------------------------------------------------------------

export interface BoardStatus {
    state: string;           // scanning | connected | failed | reconnecting
    connected: boolean;
    scanning: boolean;
    failed: boolean;
    reconnecting: boolean;
    target: string;
    advertised: string;
    idle: boolean;
    swpcol: boolean;
    numHolds: number;
    version: string;
}

export interface Hold {
    holdnum: number;
    x: number;
    y: number;
    r: number;
    g: number;
    b: number;
}

export interface PublishResult {
    sent: number;
    connected: boolean;
    state: string;
}

export interface Settings {
    swpcol: boolean;
    idle: boolean;
    advname: string;
    tgtname: string;
}

const api = async <T,>(path: string, init?: RequestInit): Promise<T> => {
    const res = await fetch(path, {
        headers: { "Content-Type": "application/json" },
        ...init,
    });
    if (!res.ok) {
        throw new Error(`${init?.method ?? "GET"} ${path} -> HTTP ${res.status}`);
    }
    return (await res.json()) as T;
};

export const useBoardApi = () => {
    const [status, setStatus] = useState<BoardStatus | null>(null);
    // PixelMapping[y][x] => holdnum (or -1 when there is no hold at that cell)
    const [grid, setGrid] = useState<number[][]>([]);
    const [holds, setHolds] = useState<Hold[]>([]);
    const [error, setError] = useState<string | null>(null);

    const refreshStatus = useCallback(async () => {
        try {
            const res = await api<{ status: BoardStatus }>("/api/status");
            setStatus(res.status);
            setError(null);
        } catch (e: any) {
            setError(e?.message ?? String(e));
        }
    }, []);

    const loadGrid = useCallback(async () => {
        try {
            const res = await api<{ cols: number; rows: number; mapping: number[][] }>("/api/grid");
            const mapping: number[][] = [];
            for (let y = 0; y < res.rows; y++) {
                mapping.push(new Array(res.cols).fill(-1));
            }
            for (const entry of res.mapping) {
                const [x, y, holdnum] = entry;
                mapping[y][x] = holdnum;
            }
            setGrid(mapping);
        } catch (e: any) {
            setError(e?.message ?? String(e));
        }
    }, []);

    const refreshHolds = useCallback(async () => {
        try {
            const res = await api<{ holds: Hold[] }>("/api/holds");
            setHolds(res.holds);
        } catch (e: any) {
            setError(e?.message ?? String(e));
        }
    }, []);

    const publishHolds = useCallback(
        async (pixels: { holdnum: number; r: number; g: number; b: number }[]): Promise<PublishResult> => {
            const res = await api<PublishResult>("/api/holds", {
                method: "POST",
                body: JSON.stringify({ holds: pixels }),
            });
            await refreshStatus();
            return res;
        },
        [refreshStatus]
    );

    const clearHolds = useCallback(async () => {
        await api<{ sent: number; connected: boolean }>("/api/holds", { method: "DELETE" });
        await refreshStatus();
    }, [refreshStatus]);

    const setSettings = useCallback(async (settings: { swpcol?: boolean; idle?: boolean; advname?: string }) => {
        await api<{ ok: boolean }>("/api/settings", {
            method: "POST",
            body: JSON.stringify(settings),
        });
        await refreshStatus();
    }, [refreshStatus]);

    // initial load + status polling (REST, no websocket needed)
    useEffect(() => {
        loadGrid();
        refreshStatus();
        refreshHolds();
        const timer = setInterval(refreshStatus, 2000);
        return () => clearInterval(timer);
    }, [loadGrid, refreshStatus, refreshHolds]);

    return {
        status,
        grid,
        holds,
        error,
        refreshStatus,
        refreshHolds,
        publishHolds,
        clearHolds,
        setSettings,
    };
};

export default useBoardApi;