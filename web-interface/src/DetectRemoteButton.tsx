import { Button, Form, Modal } from "react-bootstrap";
import { useState } from "react";
import { useToaster } from "./toaster";
import { Spinner } from './Spinner';

export function DetectRemoteButton(props: { onSaved: () => void } ) : JSX.Element {

    const [showDetect, setShowDetect] = useState(false);
    const [newRemoteName, setNewRemoteName] = useState("");
    const [detecting, setDetecting] = useState(false);
    const [detectionResults, setDetectionResults] = useState<number[]>([]);
    const [importId, setImportId] = useState(0);
    const toaster = useToaster();
    
    const handleDetectCancel = () => {
        setShowDetect(false);
        setNewRemoteName("");
        setImportId(0);
    };

    const startDetect = async () => {

        setShowDetect(true);
        let response = await fetch("/api/remotes/discover.json?start=1");

        setDetecting(true);
        let body: boolean =  await response.json();
        if(!body)
        {
            toaster.open("Unable to add remote", "For some reason, the remote could not be added.");
            setDetecting(false);
        }
        else
        {
            await new Promise(resolve => setTimeout(resolve, 10_000));

            let resultsResponse = await fetch("/api/remotes/results.json");
            let idList: number[] = await resultsResponse.json();
            setDetecting(false);
            setDetectionResults(idList);
        }
    }

    const doImport = async () => {
        if(newRemoteName.length < 1 || newRemoteName.length > 47)
        {
            toaster.open("Invalid name", "You need to specify a valid name for the remote to create");
            return;
        }
        if(importId === -1)
        {
            toaster.open("Invalid Remote ID", "You need to select a Remote ID to import from the dropdown.");
            return;
        }

        const params = new URLSearchParams();
        params.set("name", newRemoteName);
        params.set("id", importId.toString());
        let response = await fetch("/api/remotes/import.json?" + params.toString());

        let body: boolean =  await response.json();
        if(!body)
        {
            toaster.open("Unable to import remote", "For some reason, the remote could not be added.");
        }
        else
        {
            setNewRemoteName("");
            setShowDetect(false);
            props.onSaved();
        }
    };

return (
    <>
        <Button className="mt-4 ms-4" onClick={startDetect}>Import External Remote</Button>
        <Modal show={showDetect} onHide={handleDetectCancel}>
            <Modal.Header closeButton>
                <Modal.Title>Import an External Remote</Modal.Title>
            </Modal.Header>
            <Modal.Body>
                <Spinner loading={detecting}></Spinner>
                <div>Press buttons on the remote to import now!</div>
                <Form hidden={detecting}>
                    <Form.Group className="mb-3">
                        <Form.Select value={importId} onChange={e => setImportId(e.target.value===""? -1 :parseInt(e.target.value))}>
                                    <option value="-1" key={-1}>Choose the remote to import. If the list is empty, try pressing some buttons on the remote.</option>
                                    {detectionResults.map(b => (<option key={b} value={b}>{b.toString()}</option>))}
                        </Form.Select>
                    </Form.Group>
                    <Form.Group className="mb-3">
                        <Form.Label htmlFor="remoteName">Remote Name</Form.Label>
                            <Form.Control
                                type="text"
                                aria-describedby="remoteNameHelpBlock"
                                id="remoteName"
                                value={newRemoteName}
                                onChange={e => setNewRemoteName(e.target.value )} />
                        <Form.Text id="remoteNameHelpBlock" muted>
                            The name will be used to identify the remote in Home Assistant. Between 1 and 47 characters
                        </Form.Text>
                    </Form.Group>
                </Form>
            </Modal.Body>
            <Modal.Footer>
                <Button variant="secondary" onClick={handleDetectCancel}>
                    Cancel
                </Button>
                <Button variant="primary" onClick={doImport}>
                    Add
                </Button>
            </Modal.Footer>
        </Modal>
    </>
);
}
