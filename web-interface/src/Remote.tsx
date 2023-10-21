

import React, { useState } from 'react';

import './Remote.css';
import upIcon from './up.svg';
import downIcon from './down.svg';


import {RemoteConfig} from './BlindTypes';
import { Button, Card, Col, Collapse, Container, Form, InputGroup, Modal, Row, Stack } from 'react-bootstrap';
import { useToaster } from './toaster';

export function RemoteButtons(props: {config: RemoteConfig}) {

    return (
            <Card bg="light">
                <Stack>
                    <Button variant="light" className="m-3" >
                    <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" className="bi bi-arrow-up-circle" viewBox="0 0 16 16">
                        <path fill-rule="evenodd" d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0zm-7.5 3.5a.5.5 0 0 1-1 0V5.707L5.354 7.854a.5.5 0 1 1-.708-.708l3-3a.5.5 0 0 1 .708 0l3 3a.5.5 0 0 1-.708.708L8.5 5.707V11.5z"/>
                    </svg>
                    </Button>

                    <Button variant="light" className="m-3" >
                        <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" className="bi bi-dash-circle" viewBox="0 0 16 16">
                            <path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14zm0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16z"/>
                            <path d="M4 8a.5.5 0 0 1 .5-.5h7a.5.5 0 0 1 0 1h-7A.5.5 0 0 1 4 8z"/>
                        </svg>
                    </Button>
                    <Button variant="light" className="mx-auto mt-3">
                        <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" className="bi bi-arrow-down-circle" viewBox="0 0 16 16">
                            <path fill-rule="evenodd" d="M1 8a7 7 0 1 0 14 0A7 7 0 0 0 1 8zm15 0A8 8 0 1 1 0 8a8 8 0 0 1 16 0zM8.5 4.5a.5.5 0 0 0-1 0v5.793L5.354 8.146a.5.5 0 1 0-.708.708l3 3a.5.5 0 0 0 .708 0l3-3a.5.5 0 0 0-.708-.708L8.5 10.293V4.5z"/>
                        </svg>
                    </Button>
                    <Button variant="light" className="ms-auto" size="sm">
                        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" fill="currentColor" className="bi bi-three-dots" viewBox="0 0 16 16">
                            <path d="M3 9.5a1.5 1.5 0 1 1 0-3 1.5 1.5 0 0 1 0 3zm5 0a1.5 1.5 0 1 1 0-3 1.5 1.5 0 0 1 0 3zm5 0a1.5 1.5 0 1 1 0-3 1.5 1.5 0 0 1 0 3z"/>
                        </svg>
                    </Button>
                </Stack>
            </Card>
        );
}

export function Remote(props: {config: RemoteConfig, onChanged: () => void}) {

    const [newName, setNewName] = useState(props.config.name);
    const [showPopup, setShowPopup] = useState(false);
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
        let response = await fetch("/api/blinds/update.json?" + params.toString());

        let body: boolean =  await response.json();
        if(!body)
        {
            toaster.open("Command Rejected", "Something went wrong with the command");
        }

        setShowPopup(false);
    };

    return (
        <>
            <Col className="align-self-end">
                <Card className="remote-tile">
                    <Card.Header>{props.config.name}</Card.Header>
                    <Card.Body>
                        <Stack gap={4}>
                        <RemoteButtons config={props.config} />
                        <Button className="mx-auto" variant="outline-secondary" size="sm" onClick={handleEdit}>Edit ✏️</Button>
                        </Stack>
                    </Card.Body>
                </Card>
            </Col>
            <Modal show={showPopup} onHide={handleClose}>
            <Modal.Header closeButton>
                        <Modal.Title>Edit Remote</Modal.Title>
                    </Modal.Header>
                    <Modal.Body>
                        <InputGroup className="mb-3">
                            <Form.Control placeholder="Remote name" aria-label="Remote name" aria-describedby="basic-addon2"
                                value={newName} onChange={e => setNewName(e.target.value)} />
                            <Button id="button-addon2" onClick={handleSave} disabled={newName == props.config.name}>
                                Save
                            </Button>
                        </InputGroup>  
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