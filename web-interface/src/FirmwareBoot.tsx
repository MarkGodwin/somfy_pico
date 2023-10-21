import { MouseEvent } from 'react';
import { useToaster } from './toaster';


export function FirmwareBoot() : JSX.Element {


  const toaster = useToaster();


  async function doReboot(e: MouseEvent<HTMLButtonElement>) {
    e.preventDefault();

    //setLoading(true);
    const params = new URLSearchParams();
    params.set("mode", "firmware");
    let response = await fetch("/api/configure.json?" + params.toString());
    let body: boolean =  await response.json();
    if(body)
      toaster.open("Device Rebooting...", "The device should now show as a USB device, ready for firwmware update. This page will no longer work until the device is updated, or power-cycled.");
    else
      toaster.open("Failed to Reboot", "Something went wrong. Try the BOOTSEL button instead.");
  }

    return (
        <div className="card my-5">
          <div className="card-body">
            <h5 className="card-title">Firmware Update</h5>
            <h6 className="card-subtitle mb-2 text-muted">Advanced use only!</h6>

            <form method="get" action='toast' className="mt-3" >
              <div className="mb-3">
                <div className="mb-3">
                  <button type="submit" className="btn btn-primary" onClick={doReboot}>Reboot in Firmware Mode</button>
                </div>
                <span id="ssidHelpInline" className="form-text">
                  Reboot the device into Firware update mode. Connect the device to a PC before pressing this button. The device will be mounted as a USB drive.
                </span>
              </div>
            </form>
          </div>
        </div>
    );
  }
