import React, { useState } from 'react';

import './Remote.css';

import { BlindConfig, RemoteConfig } from './BlindTypes';
import { Button, ButtonGroup, Card, Col, Form, InputGroup, ListGroup, Modal, Stack } from 'react-bootstrap';
import { useToaster } from './toaster';
import { SomfyButton, pressButtons, useRemoteApi } from './remoteApi';

export function RemoteButtons(props: { config: RemoteConfig }) {

    const [isBusy, doButtonPress] = useRemoteApi(props.config.id);
    const [showFull, setShowFull] = useState(false);

    const upDownControls = (
        <>
            <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Up, false)} >
                <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" className="bi bi-arrow-up-circle" viewBox="0 0 16 16">
                    <path fill-rule="evenodd" d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0zm-7.5 3.5a.5.5 0 0 1-1 0V5.707L5.354 7.854a.5.5 0 1 1-.708-.708l3-3a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 5.707V11.5z" />
                </svg>
            </Button>

            <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.My, false)} >
                <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" className="bi bi-dash-circle" viewBox="0 0 16 16">
                    <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z" />
                    <path d="M4 8a.5.5 0 0 1 .5-.5h7a.5.5 0 0 1 0 1h-7A.5.5 0 0 1 4 8z" />
                </svg>
            </Button>
            <Button variant="light" className="mx-auto mt-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Down, false)} >
                <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" className="bi bi-arrow-down-circle" viewBox="0 0 16 16">
                    <path fill-rule="evenodd" d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0zM8.5 4.5a.5.5 0 0 0-1 0v5.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V4.5z" />
                </svg>
            </Button>
        </>
    )

    return (
        <Card bg="light">
            <Stack>
                {upDownControls}
                <Button variant="light" className="ms-auto" size="sm" onClick={() => setShowFull(true)}>
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-three-dots" viewBox="0 0 16 16">
                        <path d="M3 9.5a1.5 1.5 0 1 1 0-3 1.5 1.5 0 0 1 0 3zm5 0a1.5 1.5 0 1 1 0-3 1.5 1.5 0 0 1 0 3zm5 0a1.5 1.5 0 1 1 0-3 1.5 1.5 0 0 1 0 3z" />
                    </svg>
                </Button>
            </Stack>
            <Modal show={showFull} onHide={() => setShowFull(false)}>
                <Modal.Header closeButton>{props.config.name}</Modal.Header>
                <Modal.Body>
                    <Card bg="light">
                        <Stack direction="horizontal">
                            <Stack>
                                {upDownControls}
                            </Stack>
                            <Stack>
                                <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Up, true)} >
                                    <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M16 8A8 8 0 1 0 0 8a8 8 0 0 0 16 0zm-7.5 3.5a.5.5 0 0 1-1 0V5.707L5.354 7.854a.5.5 0 1 1-.708-.708l3-3a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 5.707V11.5z" />
                                    </svg>
                                </Button>
                                <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.My, true)} >
                                    <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M16 8A8 8 0 1 1 0 8a8 8 0 0 1 16 0zM4.5 7.5a.5.5 0 0 0 0 1h7a.5.5 0 0 0 0-1h-7z" />
                                    </svg>
                                </Button>
                                <Button variant="light" className="mx-auto mt-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Down, true)} >
                                    <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M16 8A8 8 0 1 1 0 8a8 8 0 0 1 16 0zM8.5 4.5a.5.5 0 0 0-1 0v5.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V4.5z" />
                                    </svg>
                                </Button>
                            </Stack>
                            <Stack>
                                <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Up | SomfyButton.Down, true)} aria-description='Up+Down Long' >
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="32" fill="currentColor"  viewBox="0 0 16 16">
                                        <path d="M16 8A8 8 0 1 0 0 8a8 8 0 0 0 16 0zm-7.5 3.5a.5.5 0 0 1-1 0V5.707L5.354 7.854a.5.5 0 1 1-.708-.708l3-3a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 5.707V11.5z"/>
                                    </svg>
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M8 4a.5.5 0 0 1 .5.5v3h3a.5.5 0 0 1 0 1h-3v3a.5.5 0 0 1-1 0v-3h-3a.5.5 0 0 1 0-1h3v-3A.5.5 0 0 1 8 4z"/>
                                    </svg>
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="32" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M16 8A8 8 0 1 1 0 8a8 8 0 0 1 16 0zM8.5 4.5a.5.5 0 0 0-1 0v5.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V4.5z"/>
                                    </svg>
                                </Button>
                                <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Up | SomfyButton.My, false)} aria-description='Up+Stop Short'  >
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="32" fill="currentColor" className="bi bi-arrow-up-circle" viewBox="0 0 16 16">
                                        <path fill-rule="evenodd" d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0zm-7.5 3.5a.5.5 0 0 1-1 0V5.707L5.354 7.854a.5.5 0 1 1-.708-.708l3-3a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 5.707V11.5z" />
                                    </svg>
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M8 4a.5.5 0 0 1 .5.5v3h3a.5.5 0 0 1 0 1h-3v3a.5.5 0 0 1-1 0v-3h-3a.5.5 0 0 1 0-1h3v-3A.5.5 0 0 1 8 4z"/>
                                    </svg>
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="32" fill="currentColor" className="bi bi-dash-circle" viewBox="0 0 16 16">
                                        <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z" />
                                        <path d="M4 8a.5.5 0 0 1 .5-.5h7a.5.5 0 0 1 0 1h-7A.5.5 0 0 1 4 8z" />
                                    </svg>
                                </Button>
                                <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Down | SomfyButton.My, false)} aria-description='Down+Stop Short'  >
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="32" fill="currentColor" className="bi bi-arrow-down-circle" viewBox="0 0 16 16">
                                        <path fill-rule="evenodd" d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0zM8.5 4.5a.5.5 0 0 0-1 0v5.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V4.5z" />
                                    </svg>
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M8 4a.5.5 0 0 1 .5.5v3h3a.5.5 0 0 1 0 1h-3v3a.5.5 0 0 1-1 0v-3h-3a.5.5 0 0 1 0-1h3v-3A.5.5 0 0 1 8 4z"/>
                                    </svg>
                                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="32" fill="currentColor" className="bi bi-dash-circle" viewBox="0 0 16 16">
                                        <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z" />
                                        <path d="M4 8a.5.5 0 0 1 .5-.5h7a.5.5 0 0 1 0 1h-7A.5.5 0 0 1 4 8z" />
                                    </svg>
                                </Button>
                            </Stack>
                            <Stack>
                                <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Prog, false)} aria-description='Prog Short'  >
                                    <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M12.496 8a4.491 4.491 0 0 1-1.703 3.526L9.497 8.5l2.959-1.11c.027.2.04.403.04.61Z" />
                                        <path d="M16 8A8 8 0 1 1 0 8a8 8 0 0 1 16 0Zm-1 0a7 7 0 1 0-13.202 3.249l1.988-1.657a4.5 4.5 0 0 1 7.537-4.623L7.497 6.5l1 2.5 1.333 3.11c-.56.251-1.18.39-1.833.39a4.49 4.49 0 0 1-1.592-.29L4.747 14.2A7 7 0 0 0 15 8Zm-8.295.139a.25.25 0 0 0-.288-.376l-1.5.5.159.474.808-.27-.595.894a.25.25 0 0 0 .287.376l.808-.27-.595.894a.25.25 0 0 0 .287.376l1.5-.5-.159-.474-.808.27.596-.894a.25.25 0 0 0-.288-.376l-.808.27.596-.894Z" />
                                    </svg>
                                </Button>
                                <Button variant="light" className="mx-auto m-3" disabled={isBusy} onClick={() => doButtonPress(SomfyButton.Prog, true)} aria-description='Prog Long' >
                                    <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" viewBox="0 0 16 16">
                                        <path d="M6.705 8.139a.25.25 0 0 0-.288-.376l-1.5.5.159.474.808-.27-.595.894a.25.25 0 0 0 .287.376l.808-.27-.595.894a.25.25 0 0 0 .287.376l1.5-.5-.159-.474-.808.27.596-.894a.25.25 0 0 0-.288-.376l-.808.27.596-.894Z"/>
                                        <path d="M8 16A8 8 0 1 0 8 0a8 8 0 0 0 0 16Zm-6.202-4.751 1.988-1.657a4.5 4.5 0 0 1 7.537-4.623L7.497 6.5l1 2.5 1.333 3.11c-.56.251-1.18.39-1.833.39a4.49 4.49 0 0 1-1.592-.29L4.747 14.2a7.031 7.031 0 0 1-2.949-2.951ZM12.496 8a4.491 4.491 0 0 1-1.703 3.526L9.497 8.5l2.959-1.11c.027.2.04.403.04.61Z"/>
                                    </svg>
                                </Button>
                            </Stack>
                        </Stack>
                    </Card>
                    <div className="mt-4">
                        <span>
                            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z"/></svg> - Short Press
                        </span>
                        <span className="ms-4">
                            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" viewBox="0 0 16 16"><circle cx="8" cy="8" r="8"/></svg> - Long Press
                        </span>
                    </div>
                </Modal.Body>
                <Modal.Footer>
                    <Button variant="secondary" onClick={() => setShowFull(false)}>
                        Close
                    </Button>
                </Modal.Footer>
            </Modal>
        </Card>
    );
}

export function Remote(props: {
    config: RemoteConfig,
    blinds: BlindConfig[],
    onChanged: () => void
 }) {

    const [newName, setNewName] = useState(props.config.name);
    const [showPopup, setShowPopup] = useState(false);
    const [bindTo, setBindTo] = useState(-1);
    const [isBinding, setIsBinding] = useState(false);
    const toaster = useToaster();

    const handleEdit = (e: React.MouseEvent<HTMLButtonElement>) => {
        setNewName(props.config.name);
        setShowPopup(true);
    }
    const handleClose = () => setShowPopup(false);
    const handleSave = async () => {

        const params = new URLSearchParams();
        params.set("id", props.config.id.toString());
        params.set("name", newName);
        let response = await fetch("/api/remotes/update.json?" + params.toString());

        let body: boolean = await response.json();
        if (!body) {
            toaster.open("Command Rejected", "Something went wrong with the command");
        }

        props.onChanged();
        setShowPopup(false);
    };

    const handleBind = async () => {

        setIsBinding(true);
        try {
        // Get the blind's remote
        const target = props.blinds.find(b => b.id === bindTo);
        if(!target)
            return;

        // Long Press the Program button on the blind's remote
        if(!await pressButtons(target.remoteId, SomfyButton.Prog, true))
        {
            toaster.open("Bind Failed", "Unable to ask the existing blind to enter program mode.");
            return;
        }
        
        await new Promise(resolve => setTimeout(resolve, 3000));

        // Short Press the Program button on this remote
        if(!await pressButtons(props.config.id, SomfyButton.Prog, false))
        {
            toaster.open("Bind Failed", "This remote rejected the bind command.");
            return;
        }
        await new Promise(resolve => setTimeout(resolve, 1000));

        // Tell the controller the remote is bound.
        const params = new URLSearchParams();
        params.set("id", props.config.id.toString());
        params.set("blindId", bindTo.toString());
        let response = await fetch("/api/remotes/bindBlind.json?" + params.toString());

        let body: boolean = await response.json();
        if(body)
        {
            toaster.open("Bind Successful", "The blind should now be controlled by this remote.");
            props.onChanged();
        }
        else
            toaster.open("Bind Error", "The blind should now be controlled by this remote, but the controller won't know about it.");
        }
        finally
        {
            setIsBinding(false);
        }
    };
    const handleUnbind = async (blindId: number) => {

        setIsBinding(true);
        try {
            // Get the blind's remote
            const target = props.blinds.find(b => b.id === blindId);
            if(!target)
                return;

            // Long Press the Program button on the blind's remote
            if(!await pressButtons(target.remoteId, SomfyButton.Prog, true))
            {
                toaster.open("Unbind Failed", "Unable to ask the existing blind to enter program mode.");
                return;
            }
            
            await new Promise(resolve => setTimeout(resolve, 3000));

            // Long Press the Program button on this remote
            if(!await pressButtons(props.config.id, SomfyButton.Prog, true))
            {
                toaster.open("Unbind Failed", "This remote rejected the bind command.");
                return;
            }
            await new Promise(resolve => setTimeout(resolve, 1000));

            // Tell the controller the remote is not bound.
            const params = new URLSearchParams();
            params.set("id", props.config.id.toString());
            params.set("blindId", bindTo.toString());
            let response = await fetch("/api/remotes/unbindBlind.json?" + params.toString());

            let body: boolean = await response.json();
            if(body)
            {
                toaster.open("Unbind Successful", "The blind should now no longer be controlled by this remote.");
                props.onChanged();
            }
            else
                toaster.open("Unbind Error", "The blind should now no longer be controlled by this remote, but the controller won't know about it.");
        }
        finally{
            setIsBinding(false);
        }
        
    };

    const handleDelete = async () => {

        if(!window.confirm("Deleting a remote cannot be undone.\nAre you absolutely sure?"))
            return;


        const params = new URLSearchParams();
        params.set("id", props.config.id.toString());
        let response = await fetch("/api/remotes/delete.json?" + params.toString());

        let body: boolean =  await response.json();
        if(!body)
            toaster.open("Delete Failed", "For some reason, the controller didn't delete the blind.");
        else
            props.onChanged();
    }

    return (
        <>
            <Col className="align-self-end">
                <Card className="remote-tile">
                    <Card.Header>{props.config.name}</Card.Header>
                    <Card.Body>
                        <Stack gap={4}>
                            <RemoteButtons config={props.config} />
                            <ButtonGroup>
                                <Button className="mx-auto" variant="outline-secondary" size="sm" onClick={handleEdit}>Edit ✏️</Button>
                                <Button className="mx-auto" variant="outline-secondary" size="sm" onClick={handleDelete} hidden={props.config.blinds.length !== 0}>Delete 🗑️</Button>
                            </ButtonGroup>
                        </Stack>
                    </Card.Body>
                </Card>
            </Col>
            <Modal show={showPopup} onHide={handleClose}>
                <Modal.Header closeButton>
                    <Modal.Title>Edit Remote</Modal.Title>
                </Modal.Header>
                <Modal.Body>
                    <div>
                        <InputGroup className="mb-3">
                            <Form.Control placeholder="Remote name" aria-label="Remote name" aria-describedby="basic-addon2"
                                value={newName} onChange={e => setNewName(e.target.value)} />
                            <Button id="button-addon2" onClick={handleSave} disabled={newName === props.config.name}>
                                Save
                            </Button>
                        </InputGroup>
                        <InputGroup className="mb-3">
                            <Form.Select value={bindTo} onChange={e => setBindTo(e.target.value===""? -1 :parseInt(e.target.value))}>
                                <option value="-1">Select a blind to bind</option>
                                {props.blinds.filter(b => props.config.blinds.indexOf(b.id) === -1).map(b => (<option value={b.id}>{b.name}</option>))}
                            </Form.Select>
                            <Button id="button-addon3" onClick={handleBind} disabled={isBinding || bindTo === -1}>
                                    Bind
                            </Button>
                        </InputGroup>
                        <h5>Associated Blinds:</h5>
                        <ListGroup>
                            {
                                props.config.blinds.map(id => props.blinds.find(b => b.id === id)).filter(b => b).map(b => (
                                    <ListGroup.Item>
                                        <Button className="float-end btn-sm"  onClick={() => handleUnbind(b!.id)} disabled={isBinding}>Unbind</Button>
                                        <div className="me-auto">{b!.name}</div>
                                    </ListGroup.Item>
                                ))
                            }
                        </ListGroup>
                    </div>
                </Modal.Body>
                <Modal.Footer>
                    <Button variant="secondary" onClick={handleClose}>
                        Close
                    </Button>
                </Modal.Footer>
            </Modal>
        </>
    );
}