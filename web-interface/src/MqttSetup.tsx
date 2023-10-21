import { MouseEvent, useEffect, useState } from 'react';
import { matchPath } from 'react-router-dom';
import { useToaster } from './toaster';
import { Spinner } from './Spinner';

interface MqttCfg {
  address: string,
  port: number,
  username: string,
  password: string,
  topic: string
};

export function MqttSetup() : JSX.Element {

  const toaster = useToaster();

  const [loading, setLoading] = useState(true);
  const [mqttCfg, setMqttCfg] = useState<MqttCfg|null>(null);
  
  useEffect(() => {
    let cancelled = false;
    let timeout: number | undefined = undefined;

    async function getMqtt() {
      const response : MqttCfg = await (await fetch("/api/mqtt.json")).json();

      if(!cancelled)
      {
        setLoading(false);
        setMqttCfg(response);
      }
    };

    getMqtt();
    return () => {
        cancelled = true;
        window.clearTimeout(timeout);
    };
  }, []);

  async function doSave(e: MouseEvent<HTMLButtonElement>) {
    e.preventDefault();

    if(!mqttCfg)
      return;
    if(!mqttCfg.address.match(/^(?:[0-9]{1,3}\.){3}[0-9]{1,3}$/))
    {
      toaster.open('IP address invalid', 'You need to enter the broker in IPv4 dotted format.');
      return;
    }
    if(mqttCfg.port < 1 || mqttCfg.port > 65535)
    {
      toaster.open('Port invalid',
        (<><p>The MQTT port isn't within the valid range</p><p>The most common MQTT port is 1883</p></>));
      return;
    }

    setLoading(true);
    const params = new URLSearchParams();
    params.set("mode", "mqtt");
    params.set("host", mqttCfg.address);
    params.set("port", mqttCfg.port.toString());
    params.set("username", mqttCfg.username);
    params.set("password", mqttCfg.password);
    params.set("topic", mqttCfg.topic);
    let response = await fetch("/api/configure.json?" + params.toString());
    let body: boolean =  await response.json();
    setLoading(false);
    if(body)
      toaster.open("Saved Successfully", <><p>The device will now reboot, and connect to MQTT!</p><p>Give it a few seconds, and home assistant should start to discover any blinds you have configured.</p></>);
    else
      toaster.open("Failed to Save", "Something went wrong, and the device couldn't save your MQTT settings.");

  }

  return (
    <div className="card my-5">
      <div className="card-body">
        <h5 className="card-title">MQTT Setup</h5>
        <h6 className="card-subtitle mb-2 text-muted">Configure the Somfy Blind Controller to connect to your MQTT Broker</h6>

        <form method="get" className="mt-3"  >
          <fieldset disabled={loading}>
            <div className="mb-3">
              <label htmlFor="mqttBroker" className="form-label">MQTT Broker IP Address and Port number</label>
              <div className="row">
                <div className="col-9">
                  <input id="mqttAddress" type="input" className="form-control col-9" aria-describedby="mqttUserHelp" value={mqttCfg?.address} onChange={e => { setMqttCfg( { ...mqttCfg!, address: e.target.value }); }} />
                </div>
                <div className="col-3">
                  <input id="mqttPort" type="number" className="form-control col-3" aria-label="Port" placeholder="1883" value={mqttCfg?.port} onChange={e => { setMqttCfg( { ...mqttCfg!, port: parseInt(e.target.value) }); }} />
                </div>
              </div>
              <span id="mqttAddress" className="form-text">
                Enter the IP Address and port number of the MQTT Broker to connect to. Host Names are not supported!
              </span>
            </div>

            <div className="mb-3">
              <label htmlFor="mqttUser" className="form-label">MQTT User</label>
              <input id="mqttUser" type="input" className="form-control" aria-describedby="mqttUserHelp" value={mqttCfg?.username} onChange={e => { setMqttCfg( { ...mqttCfg!, username: e.target.value }); }} />
              <span id="mqttUserHelp" className="form-text">
                Enter the username for the MQTT Broker
              </span>
            </div>

            <div className="mb-3">
              <label htmlFor="mqttPassword" className="form-label">MQTT Password</label>
              <input id="inputPassword" type="password" className="form-control" aria-describedby="passwordHelpInline" value={mqttCfg?.password} onChange={e => { setMqttCfg( { ...mqttCfg!, password: e.target.value }); }} />
              <span id="ssidHelpInline" className="form-text">
                [Optional] Enter the password for the MQTT Broker
              </span>
            </div>

            <div className="mb-3">
              <label htmlFor="mqttUser" className="form-label">Discovery Topic</label>
              <input id="mqttUser" type="input" className="form-control" aria-describedby="mqttUserHelp" value={mqttCfg?.topic} onChange={e => { setMqttCfg( { ...mqttCfg!, topic: e.target.value }); }} />
              <span id="mqttUserHelp" className="form-text">
                [Optional] Enter the Home Assistant discovery topic from the HA MQTT Setup. This is usually "homeassistant". Leave blank to disable discovery (not reccomended).
              </span>
            </div>

            <div className="mb-3">
              <button type="submit" className="btn btn-primary" onClick={doSave}>Save and Reboot</button>
              <Spinner loading={loading} />
            </div>
          </fieldset>
        </form>
      </div>
    </div>
  );
}
