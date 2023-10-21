import { Button, Form, Modal } from "react-bootstrap";
import { useState } from "react";
import { useToaster } from "./toaster";

export function AddRemoteButton(props: { onSaved: () => void } ) : JSX.Element {

    const [showAdd, setShowAdd] = useState(false);
    const [newRemoteName, setNewRemoteName] = useState("");
    const toaster = useToaster();
    
    const handleAddCancel = () => {
        setShowAdd(false);
        setNewRemoteName("");
    };

    const doAdd = async () => {
        if(newRemoteName.length < 1 || newRemoteName.length > 47)
        {
            toaster.open("Invalid name", "You need to specify a valid name for the blind to create");
            return;
        }

        const params = new URLSearchParams();
        params.set("name", newRemoteName);
        let response = await fetch("/api/blinds/add.json?" + params.toString());

        let body: boolean =  await response.json();
        if(!body)
        {
            toaster.open("Unable to add blind", "For some reason, the blind could not be added.");
        }
        else
        {
            setNewRemoteName("");
            setShowAdd(false);
            props.onSaved();
        }
    };

return (
    <>
        <Button className="mt-4" onClick={() => setShowAdd(true)}>Add</Button>
        <Modal show={showAdd} onHide={handleAddCancel}>
            <Modal.Header closeButton>
                <Modal.Title>Edit blind</Modal.Title>
            </Modal.Header>
            <Modal.Body>
                <Form>
                    <Form.Group className="mb-3">
                        <Form.Label htmlFor="remoteName">Blind Name</Form.Label>
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
                <Button variant="secondary" onClick={handleAddCancel}>
                    Cancel
                </Button>
                <Button variant="primary" onClick={doAdd}>
                    Add
                </Button>
            </Modal.Footer>
        </Modal>
    </>
);
}
