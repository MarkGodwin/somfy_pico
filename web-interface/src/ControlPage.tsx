
import { MouseEvent, useEffect, useState } from 'react';
import { Form } from "react-router-dom";
import { Accordion, Button, Container, Row, Stack } from "react-bootstrap";

import { useToaster } from './toaster';
import { Spinner } from "./Spinner";
import { BlindConfig, RemoteConfig } from "./BlindTypes";
import {Blind} from "./Blind";
import {Remote} from "./Remote";
import { AddBlind } from './AddBlind';

export function ControlPage() : JSX.Element {

  const [loading, setLoading] = useState(true);
  const [reload, setReload] = useState(true);
  const [blinds, setBlinds] = useState<BlindConfig[]>([]);
  const [remotes, setRemotes] = useState<RemoteConfig[]>([]);
  const [wifiPassword, setWifiPassword] = useState("********");

  const toaster = useToaster();
 
  useEffect(() => {
    setLoading(true);
    let cancelled = false;
    let timeout: number | undefined = undefined;

    async function getRemotes() {
      const response : RemoteConfig[] = await (await fetch("/api/remotes/list.json")).json();
      const blindResponse : BlindConfig[] = await (await fetch("/api/blinds/list.json")).json();

      if(!cancelled)
      {
        setRemotes(response);
        setBlinds(blindResponse);
        setLoading(false);
      }
    };

    getRemotes();

    return () => {
      cancelled = true;
    };
  }, [reload]);


  // Blinds and their associated remotes
  const blindList = blinds.map(i => <Blind key={i.id} config={i} remote={remotes.find(r => r.id === i.remoteId)!} onSaved={() => setReload(true)} />);

  // Other remotes
  const remoteList = remotes.filter(r => !blinds.find(b => b.remoteId === r.id)).map(r => <Remote key={r.id} config={r} onChanged={()=> setReload(true)} />);

  function addBlind(e: MouseEvent<HTMLButtonElement>) {
    toaster.open("TODO", "Add a blind. Somehow.");
  }

  function addRemote(e: MouseEvent<HTMLButtonElement>) {
    toaster.open("TODO", "Add a remote. Somehow.");
  }

  return (
    <div id="Control">
      <h1 className="mt-4">
        Blinds
      </h1>
      <Spinner loading={loading}></Spinner>
      <Accordion>
        {blindList}
        <AddBlind onSaved={() => setReload(true)} />
      </Accordion>
      <h1 className="mt-4">
        Custom Remotes
      </h1>
      <Spinner loading={loading}></Spinner>
      <Container>
        <Row xs="auto" className="align-bottom">
        {remoteList}
        {remoteList}
        {remoteList}
        </Row>
        </Container>
      <Button className="mt-4" onClick={addRemote}>Add a Remote</Button>
    </div>
  );
}