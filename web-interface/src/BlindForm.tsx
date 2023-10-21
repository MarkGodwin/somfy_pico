import { Form } from "react-bootstrap";
import { BlindConfig } from "./BlindTypes";
import { useState } from "react";


export interface BlindValues {
    name: string;
    openTime: number;
    closeTime: number;
  };

export function BlindForm(props: {config: BlindValues, onChange: (x: BlindValues) => void }) : JSX.Element
{
    const [values, setValues] = useState<BlindValues>(props.config);

    const doSetValues = (v: BlindValues) => {
        props.onChange(v);
        setValues(v);
    };

return (
    <Form>
        <Form.Group className="mb-3">
            <Form.Label htmlFor="blindName">Blind Name</Form.Label>
            <Form.Control
                type="text"
                id="blindName"
                aria-describedby="nameHelpBlock"
                value={values.name}
                onChange={e => doSetValues( { ...values, name: e.target.value })} />
            <Form.Text id="nameHelpBlock" muted>
                Between 1 and 47 characters, and no quotation marks!
            </Form.Text>
        </Form.Group>
        <Form.Group className="mb-3">
            <Form.Label htmlFor="openTime">Open time</Form.Label>
            <Form.Control type="int" id="openTime" aria-describedby="openTimeHelpBlock" 
                value={values.openTime} onChange={e => doSetValues( {...values, openTime: parseInt(e.target.value)})} />
            <Form.Text id="openTimeHelpBlock" muted>
                Number of seconds the blind takes to go from fully closed to fully open.
            </Form.Text>
        </Form.Group>
        <Form.Group className="mb-3" >
            <Form.Label htmlFor="closeTime">Close time</Form.Label>
            <Form.Control type="int" id="closeTime" aria-describedby="closeTimeHelpBlock" 
                value={values.closeTime} onChange={e => doSetValues( {...values, closeTime: parseInt(e.target.value)})} />
            <Form.Text id="closeTimeHelpBlock" muted>
                Number of seconds the blind takes to go from fully closed to fully open.
            </Form.Text>
        </Form.Group>
    </Form>
);    
}