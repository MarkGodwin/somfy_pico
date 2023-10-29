import { useState } from 'react';
import { useToaster } from './toaster';


export enum SomfyButton {
    My = 1,
    Up = 2,
    Down = 4,
    Prog = 8
};

export async function pressButtons(remoteId: number, buttons: SomfyButton, long: boolean) {
    const params = new URLSearchParams();
    params.set("id", remoteId.toString());
    params.set("buttons", buttons.toString());
    params.set("long", long ? "true" : "false");
    let response = await fetch("/api/remotes/command.json?" + params.toString());
    let body: boolean =  await response.json();
    return body;
}

export function useRemoteApi(remoteId: number, external: boolean): [boolean, (buttons: SomfyButton, long: boolean) => void]
{
    const toaster = useToaster();
    const [isBusy, setIsBusy] = useState(false);

    const pressButtonsFunc = async (buttons : SomfyButton, long: boolean) => {
        if(external && !window.confirm("Pressing buttons on imported remotes will upset the rolling code of the real remote. Are you sure?"))
            return;

        setIsBusy(true);
        const result = await pressButtons(remoteId, buttons, long);
        setIsBusy(false);
        if(!result)
        {
            toaster.open("No Click", "The device did not accept the button press.");
        }
        return;
    };

    return [isBusy, pressButtonsFunc];
}
