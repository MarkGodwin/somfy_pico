import { Accordion, Button } from "react-bootstrap";
import { BlindForm, BlindValues } from "./BlindForm";
import { useState } from "react";
import { useToaster } from "./toaster";


export function AddBlind(props: { onSaved: () => void } ) : JSX.Element {

    const [values, setValues] = useState<BlindValues>({ name: "", openTime: 20, closeTime: 30});
    const toaster = useToaster();
    const [addIndex, setAddIndex] = useState(1);

    const doAdd = async () => {
        if(values.name.length < 1 || values.name.length > 47)
        {
            toaster.open("Invalid name", "You need to specify a valid name for the blind to create");
            return;
        }

        const params = new URLSearchParams();
        params.set("name", values.name);
        params.set("openTime", values.openTime.toString());
        params.set("closeTime", values.closeTime.toString());
        let response = await fetch("/api/blinds/add.json?" + params.toString());

        let body: boolean =  await response.json();
        if(!body)
        {
            toaster.open("Unable to add blind", "For some reason, the blind could not be added.");
        }
        else
        {
            props.onSaved();
            setValues({...values, name: ""});
            setAddIndex(addIndex+1);
        }

    };
return (
    <Accordion.Item eventKey="-1">
        <Accordion.Header><strong>New Blind</strong></Accordion.Header>
        <Accordion.Body>
            <BlindForm key={addIndex} config={values} onChange={setValues} />
            <Button className="mt-3" onClick={doAdd}>Add</Button>
        </Accordion.Body>
    </Accordion.Item>
);
}
