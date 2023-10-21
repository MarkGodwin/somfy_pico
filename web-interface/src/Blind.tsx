

import {useState} from 'react';
import {Accordion, ListGroup, ProgressBar, Button, Modal, ButtonGroup} from 'react-bootstrap';

import {BlindConfig, RemoteConfig} from './BlindTypes';
import {RemoteButtons} from './Remote';
import {useToaster} from './toaster';

import './Blind.css';
import { BlindForm, BlindValues } from './BlindForm';
import { SomfyButton, pressButtons } from './remoteApi';

let dragTimeout : number | undefined;
export function Blind(props: {config: BlindConfig, remote: RemoteConfig, onSaved: () => void }) {

    const [position, setPosition] = useState(props.config.position);
    const [edit, setEdit] = useState(false);
    const [newValues, setNewValues] = useState<BlindValues>({...props.config});
    const toaster = useToaster();

    async function moveBlindToPosition(pos: number) {
        setPosition(pos);
        if(dragTimeout)
            window.clearTimeout(dragTimeout);
        dragTimeout = window.setTimeout(async () => {
            const params = new URLSearchParams();
            params.set("id", props.config.id.toString());
            params.set("command", "pos");
            params.set("payload", pos.toString());
            let response = await fetch("/api/blinds/command.json?" + params.toString());
            let body: boolean =  await response.json();
            if(!body)
                toaster.open("Command Rejected", "Something went wrong with the command");
            else
                props.onSaved();
        }, 750);
    }

    async function sendBlindCommand(cmd: string)
    {
        const params = new URLSearchParams();
        params.set("id", props.config.id.toString());
        params.set("command", "cmd");
        params.set("payload", cmd);
        let response = await fetch("/api/blinds/command.json?" + params.toString());
        let body: boolean =  await response.json();
        if(!body)
            toaster.open("Command Rejected", "Something went wrong with the command");
        else
            props.onSaved();
    }

    async function doOpen(e: React.MouseEvent<HTMLButtonElement>) {
        setPosition(100);
        await sendBlindCommand("open");

    }
    async function doClose(e: React.MouseEvent<HTMLButtonElement>) {
        setPosition(0);
        await sendBlindCommand("close");
    }


    const handleEdit = (e: React.MouseEvent<HTMLButtonElement>) => {
        e.stopPropagation();
        setNewValues({...props.config});
        setEdit(true);
    }
    const handleEditCancel = () => setEdit(false);
    const handleSave = async () => {

        const params = new URLSearchParams();
        params.set("id", props.config.id.toString());
        params.set("name", newValues!.name);
        params.set("openTime", newValues!.openTime.toString());
        params.set("closeTime", newValues!.closeTime.toString());
        let response = await fetch("/api/blinds/update.json?" + params.toString());

        let body: boolean =  await response.json();
        if(!body)
            toaster.open("Save Failed", "For some reason, the controller didn't save the changes.");
        else
            props.onSaved();

        setEdit(false);
    };

    const handleDelete = async () => {

        if(!window.confirm("Deleting the blind will delete the associated remote control, and cannot be undone.\nAre you absolutely sure?"))
            return;

        // Try to de-register the remote with the blind.
        let dereg = await pressButtons(props.config.remoteId, SomfyButton.Prog, true);
        await new Promise(resolve => setTimeout(resolve, 3000));
        dereg = dereg && await pressButtons(props.config.remoteId, SomfyButton.Prog, false);
        await new Promise(resolve => setTimeout(resolve, 3000));

        if(!dereg)
            toaster.open("Remote not deregistered", "We tried to de-register the remote with the blind, but the commands weren't accepted. Deleting the blind anyway...")

        const params = new URLSearchParams();
        params.set("id", props.config.id.toString());
        let response = await fetch("/api/blinds/delete.json?" + params.toString());

        let body: boolean =  await response.json();
        if(!body)
            toaster.open("Delete Failed", "For some reason, the controller didn't delete the blind.");
        else
            props.onSaved();

    }
    
    return (
        <Accordion.Item eventKey={props.config.id.toString()}>
            <Accordion.Header>
                {props.config.name}
            </Accordion.Header>
            <Accordion.Body>
                <div className="row">
                    <div className="col">
                        <ListGroup horizontal="sm">
                            <ListGroup.Item as='button' action={true} variant="secondary" onClick={doClose}>Close</ListGroup.Item>
                            <ListGroup.Item className="col-8" >
                                <div>
                                    <input type="range" min="0" max="100" step="1" className="form-range" value={position} onChange={e => { moveBlindToPosition(parseInt(e.target.value));}} />
                                </div>
                                <div>
                                    <ProgressBar striped variant="warning" now={props.config.position} />
                                </div>
                            </ListGroup.Item>
                            <ListGroup.Item as='button' action={true} variant="secondary" onClick={doOpen}>Open</ListGroup.Item>
                        </ListGroup>
                        
                    </div>
                    <div className="col-md-auto">
                        <RemoteButtons config={props.remote} />
                    </div>
                    <div className="col-md-auto">
                        <ButtonGroup vertical>
                            <Button variant="outline-secondary" size="sm" onClick={handleEdit}>Edit ✏️</Button>
                            <Button variant="outline-secondary" size="sm" onClick={handleDelete}>Delete 🗑️</Button>
                        </ButtonGroup>
                    </div>
                </div>
                <Modal show={edit}  onHide={handleEditCancel}>
                    <Modal.Header closeButton>
                        <Modal.Title>Edit blind</Modal.Title>
                    </Modal.Header>
                    <Modal.Body>
                        <BlindForm config={newValues} onChange={setNewValues} />
                    </Modal.Body>
                    <Modal.Footer>
                        <Button variant="secondary" onClick={handleEditCancel}>
                            Cancel
                        </Button>
                        <Button variant="primary" onClick={handleSave}>
                            Save Changes
                        </Button>
                    </Modal.Footer>
                </Modal>
            </Accordion.Body>
        </Accordion.Item>
    );
}
