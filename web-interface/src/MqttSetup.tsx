import { useEffect, useState } from 'react';

interface WifiCreds {
  currentSsid: string,
  availableSsids: string[]
};

export function MqttSetup() : JSX.Element {


    return (
        <div className="card my-5">
          <div className="card-body">
            <h5 className="card-title">MQTT Setup</h5>
            <h6 className="card-subtitle mb-2 text-muted">Configure the Somfy Blind Controller to connect to your MQTT Broker</h6>

            <form method="get" className="mt-3" >
                <div className="mb-3">
                  <label htmlFor="mqttBroker" className="form-label">MQTT Broker IP Address and Port number</label>
                  <div className="row">
                    <div className="col-9">
                      <input id="mqttAddress" type="input" className="form-control col-9" aria-describedby="mqttUserHelp" />
                    </div>
                    <div className="col-3">
                      <input id="mqttPort" type="number" className="form-control col-3" aria-label="Port" placeholder="1883"  />
                    </div>
                  </div>
                  <span id="mqttAddress" className="form-text">
                    Enter the IP Address and port number of the MQTT Broker to connect to. Host Names are not supported!
                  </span>
                </div>


                <div className="mb-3">
                  <label htmlFor="mqttUser" className="form-label">MQTT User</label>
                  <input id="mqttUser" type="input" className="form-control" aria-describedby="mqttUserHelp" />
                  <span id="mqttUserHelp" className="form-text">
                    Enter the username for the MQTT Broker
                  </span>
                </div>

                <div className="mb-3">
                  <label htmlFor="mqttPassword" className="form-label">MQTT Password</label>
                  <input id="inputPassword" type="password" className="form-control" aria-describedby="passwordHelpInline" />
                  <span id="ssidHelpInline" className="form-text">
                    [Optional] Enter the password for the MQTT Broker
                  </span>
                </div>
                <div className="mb-3">
                  <button type="submit" className="btn btn-primary">Save and Reboot</button>
                </div>
            </form>
          </div>
        </div>        
    );
  }
