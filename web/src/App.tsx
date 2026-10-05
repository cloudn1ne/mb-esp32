import * as React from "react";
import { useState } from "react";
import DrawCanvas from "./DrawCanvas";
import useBoardApi from "./hooks/useBoardApi";

export const App = () => {

    const { status, grid, holds, error, publishHolds, clearHolds, setSettings } = useBoardApi();
    const [advname, setAdvname] = useState<string>("");

    React.useEffect(() => {
        if (status) setAdvname(status.advertised);
    }, [status?.advertised]);

    const stateBadge = () => {
        if (!status) return <span className="badge rounded-pill bg-secondary status-badge">connecting…</span>;
        const cls = status.connected ? "bg-success"
            : status.reconnecting ? "bg-warning"
            : status.failed ? "bg-danger"
            : "bg-info";
        const label = status.connected ? "Connected"
            : status.reconnecting ? "Reconnecting"
            : status.failed ? "Failed"
            : "Scanning for board…";
        return <span className={`badge rounded-pill ${cls} status-badge`}>{label}</span>;
    };

    const saveSettings = async () => {
        try {
            await setSettings({ advname });
        } catch (e: any) {
            console.log("save settings failed", e);
        }
    };

    return (
        <div className="container">
            <div className="app-header">
                <img alt="Monkey Logo" src="/images/monkey1.jpeg" width="60" height="60" />
                <h1 className="mb-0">MonkeyBoard</h1>
                {stateBadge()}
                {status && <span className="text-muted">v{status.version} · {status.numHolds} holds on board</span>}
                {error && <span className="badge rounded-pill bg-danger status-badge">API error: {error}</span>}
            </div>

            <div className="row">
                <div className="col-md-9">
                    <DrawCanvas grid={grid} initialHolds={holds} onPublish={publishHolds} onClearBoard={clearHolds} />
                </div>
                <div className="col-md-3">
                    <div className="card settings-panel">
                        <div className="card-header">Settings</div>
                        <div className="card-body">
                            <div className="form-check form-switch">
                                <input className="form-check-input" type="checkbox" role="switch" id="btn_idle"
                                    checked={status?.idle ?? false}
                                    onChange={(e) => setSettings({ idle: e.target.checked })} />
                                <label className="form-check-label" htmlFor="btn_idle">Show Idle Animation</label>
                            </div>
                            <div className="form-check form-switch">
                                <input className="form-check-input" type="checkbox" role="switch" id="btn_swpcol"
                                    checked={status?.swpcol ?? false}
                                    onChange={(e) => setSettings({ swpcol: e.target.checked })} />
                                <label className="form-check-label" htmlFor="btn_swpcol">Swap Colors</label>
                            </div>
                            <hr />
                            <label htmlFor="advname" className="form-label">Board Name</label>
                            <input type="text" className="form-control form-control-sm mb-2" id="advname"
                                value={advname}
                                onChange={(e) => setAdvname(e.target.value)} />
                            <button type="button" className="btn btn-primary btn-sm" onClick={saveSettings}>Save</button>
                            <hr />
                            <div className="text-muted small">
                                Target: {status?.target ?? "…"}<br />
                                State: {status?.state ?? "…"}
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default App;