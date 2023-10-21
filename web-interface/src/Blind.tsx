

import {useContext, useState} from 'react';
import {Accordion, ListGroup, ProgressBar, Button, Form, Modal, AccordionContext} from 'react-bootstrap';

import {BlindConfig, RemoteConfig} from './BlindTypes';
import {Remote, RemoteButtons} from './Remote';
import {useToaster} from './toaster';

import './Blind.css';
import { BlindForm, BlindValues } from './BlindForm';

let dragTimeout : number | undefined;
export function Blind(props: {config: BlindConfig, remote: RemoteConfig, onSaved: () => void }) {

    const [position, setPosition] = useState(props.config.position);
    const [edit, setEdit] = useState(false);
    const [newValues, setNewValues] = useState<BlindValues>({...props.config});
    const toaster = useToaster();
    const { activeEventKey } = useContext(AccordionContext);

    async function moveBlindToPosition(pos: number) {
        setPosition(pos);
        if(dragTimeout)
            window.clearTimeout(dragTimeout);
        dragTimeout = window.setTimeout(() => {
            toaster.open("Set pos...", `Setting pos to ${pos}.`);
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
            toaster.open("Command Accepted", "Looks good.");

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
        {
            toaster.open("Command Rejected", "Something went wrong with the command");
        }

        setEdit(false);
    };
    
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
                </div>
                <div className="row mt-3">
                    <div className="col">
                        <Button variant="outline-secondary" size="sm" onClick={handleEdit}>Edit ✏️</Button>
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
