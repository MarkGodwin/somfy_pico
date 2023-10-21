import { MouseEvent, useEffect, useState } from 'react';

interface WifiCreds {
  currentSsid: string,
  availableSsids: string[]
};

export function WifiSetup() : JSX.Element {

  const [loadCount, setLoadCount] = useState(0);
  const [ssids, setSsids] = useState<string[]>([]);
  const [wifiSsid, setWifiSsid] = useState("");
  const [wifiPassword, setWifiPassword] = useState("********");
  
  useEffect(() => {
    let cancelled = false;
    let timeout: number | undefined = undefined;

    async function getSsidList() {
      const response : WifiCreds = await (await fetch("/api/wifi.json")).json();

      if(!cancelled)
      {
        setSsids(response.availableSsids);
        if(loadCount == 0) {
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
        alert("You didn't choose an SSID");
        return;
    }

    const params = new URLSearchParams();
    params.set("mode", "wifi");
    params.set("ssid", wifiSsid);
    params.set("password", wifiPassword);
    let response = await fetch("/api/configure.json?" + params.toString());
    let body: boolean =  await response.json();
    if(body)
      alert("It worked");
    else
      alert("Nope");
  }

  const availableSsidOptions = ssids.map(i => <option key={i}>{i}</option>)

    return (
        <div className="card my-5">
          <div className="card-body">
            <h5 className="card-title">WiFi Setup</h5>
            <h6 className="card-subtitle mb-2 text-muted">Configure the Somfy Blind Controller to connect to your WiFi network</h6>

            <form method="get" action='toast' className="mt-3" >
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
                  <button type="submit" className="btn btn-primary" onClick={doSave}>Save and Reboot</button>
                </div>
            </form>

          </div>
        </div>
    );
  }
