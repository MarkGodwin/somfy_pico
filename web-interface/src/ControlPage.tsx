
import { useEffect, useState } from 'react';
import { Accordion, Container, Row } from "react-bootstrap";

import { useToaster } from './toaster';
import { Spinner } from "./Spinner";
import { BlindConfig, RemoteConfig } from "./BlindTypes";
import { Blind } from "./Blind";
import { Remote } from "./Remote";
import { AddBlind } from './AddBlind';
import { AddRemoteButton } from './AddRemoteButton';
import { DetectRemoteButton } from './DetectRemoteButton';

export function ControlPage(): JSX.Element {

    const [loading, setLoading] = useState(true);
    const [reload, setReload] = useState(0);
    const [blinds, setBlinds] = useState<BlindConfig[]>([]);
    const [remotes, setRemotes] = useState<RemoteConfig[]>([]);

    const toaster = useToaster();

    useEffect(() => {
        let cancelled = false;
        let timeout: any;

        async function getRemotes() {

            try {
                const response: RemoteConfig[] = await (await fetch("/api/remotes/list.json")).json();
                const blindResponse: BlindConfig[] = await (await fetch("/api/blinds/list.json")).json();

                if (!cancelled) {
                    setRemotes(response);
                    setBlinds(blindResponse);

                    if (blindResponse.findIndex(b => b.state !== "stopped") !== -1) {
                        timeout = setTimeout(() => setReload(reload+1), 1000);
                    }
                }
            }
            catch(e: any) {
                toaster.open("Failed to read", (<><p>"The blind & remote list could not be read. Check the device is running."</p><p>{e.toString()}</p></>));
            }
            finally{
                setLoading(false);
            }
        };

        getRemotes();

        return () => {
            cancelled = true;
            clearTimeout(timeout);
        };
    }, [reload, toaster]);


    // Blinds and their associated remotes
    const blindList = blinds.map(i => <Accordion.Item eventKey={i.id.toString()}><Blind key={i.id} config={i} remote={remotes.find(r => r.id === i.remoteId)!} onSaved={() => setReload(reload+1)} /></Accordion.Item>);

    // Other remotes
    const remoteList = remotes.filter(r => !blinds.find(b => b.remoteId === r.id)).map(r => <Remote key={r.id} config={r} blinds={blinds} onChanged={() => setReload(reload+1)} />);

    return (
        <div id="Control">
            <h1 className="mt-4">
                Blinds
            </h1>
            <Spinner loading={loading}></Spinner>
            <Accordion>
                {blindList}
                <AddBlind key={-1} onSaved={() => { setReload(reload+1); }} />
            </Accordion>
            <h1 className="mt-4">
                Custom Remotes
            </h1>
            <Spinner loading={loading}></Spinner>
            <Container>
                <Row xs="auto" className="align-bottom">
                    {remoteList}
                </Row>
            </Container>
            <AddRemoteButton onSaved={() => setReload(reload+1)} />
            <DetectRemoteButton onSaved={() => setReload(reload+1)} />
        </div>
    );
}