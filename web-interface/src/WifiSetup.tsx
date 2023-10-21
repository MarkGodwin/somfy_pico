import { MouseEvent, useEffect, useState } from 'react';
import { useToaster } from './toaster';
import { Spinner } from './Spinner';

interface WifiCreds {
  currentSsid: string,
  availableSsids: string[]
};

export function WifiSetup() : JSX.Element {

  const [loading, setLoading] = useState(true);
  const [loadCount, setLoadCount] = useState(0);
  const [ssids, setSsids] = useState<string[]>([]);
  const [wifiSsid, setWifiSsid] = useState("");
  const [wifiPassword, setWifiPassword] = useState("********");

  const toaster = useToaster();
 
  useEffect(() => {
    let cancelled = false;
    let timeout: number | undefined = undefined;

    async function getSsidList() {
      const response : WifiCreds = await (await fetch("/api/wifi.json")).json();

      if(!cancelled)
      {
        setSsids(response.availableSsids);
        if(loadCount == 0) {
            setLoading(false);
            setWifiSsid(response.currentSsid);
            if(response.currentSsid === "")
               setWifiPassword("");
        }
        // Cause the SSID list to re-load
        timeout = window.setTimeout(() => {
            setLoadCount(n => n+1);
        }, 20_000);
      }
    };

    getSsidList();

    return () => {
        cancelled = true;
        window.clearTimeout(timeout);
    };
  }, [loadCount]);

  async function doSave(e: MouseEvent<HTMLButtonElement>) {
    e.preventDefault();
    if(wifiSsid === "")
    {
        toaster.open("Can't save", "You didn't choose an SSID");
        return;
    }

    setLoading(true);
    const params = new URLSearchParams();
    params.set("mode", "wifi");
    params.set("ssid", wifiSsid);
    params.set("password", wifiPassword);
    let response = await fetch("/api/configure.json?" + params.toString());
    let body: boolean =  await response.json();
    setLoading(false);
    if(body)
      toaster.open("Saved Successfully", "Wait for the device to reboot, and connect to your WiFi network to find the device.");
    else
      toaster.open("Failed to Save", "Something went wrong, and the device couldn't save your WiFi settings.");
  }

  const availableSsidOptions = ssids.map(i => <option key={i}>{i}</option>)

    return (
        <div className="card my-5">
          <div className="card-body">
            <h5 className="card-title">WiFi Setup</h5>
            <h6 className="card-subtitle mb-2 text-muted">Configure the Somfy Blind Controller to connect to your WiFi network</h6>

            <form method="get" action='toast' className="mt-3" >
              <fieldset disabled={loading}>
                  <div className="mb-3">
                    <label htmlFor="wifiSsid" className="form-label">WiFi SSID</label>
                      <input id="wifiSsid" type="text" className="form-select" aria-describedby="ssidHelpInline" value={wifiSsid} onChange={e => {setWifiSsid(e.target.value)}} list="wifiSsidList" />
                      <datalist id="wifiSsidList">
                        {availableSsidOptions}
                      </datalist>
                    <span id="ssidHelpInline" className="form-text">
                      Select the WiFi Network to connect to.
                    </span>
                  </div>            
                  <div className="mb-3">
                    <label htmlFor="wifiPassword" className="form-label" >WiFi Password</label>
                    <input id="wifiPassword" type="password" className="form-control" aria-describedby="passwordHelpInline" value={wifiPassword} onChange={e => {setWifiPassword(e.target.value)}} />
                    <span id="ssidHelpInline" className="form-text">
                      Select the WiFi Network to connect to.
                    </span>
                  </div>
                  <div className="mb-3">
                    <Spinner loading={loading}></Spinner>
                    <button type="submit" className="btn btn-primary" onClick={doSave}>Save and Reboot</button>
                  </div>
                </fieldset>
            </form>
          </div>
        </div>
    );
  }
