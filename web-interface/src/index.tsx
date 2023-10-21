import React, { useEffect, useState } from 'react';
import ReactDOM from 'react-dom/client';
import 'bootstrap/dist/css/bootstrap.css';

import {
  createHashRouter,
  createRoutesFromElements,
  Route,
  RouterProvider,
} from "react-router-dom";

import './index.css';
import {App} from './App';
import {ErrorPage} from './ErrorPage';
import {HomePage} from './HomePage';
import {ControlPage} from './ControlPage';
import {SetupPage} from './SetupPage';
import { Spinner } from 'react-bootstrap';



interface AppStatus {
  apMode: boolean;
  mqttConnected: boolean;
}

export function AppModeSwitcher() {

  const [appStatus, setAppStatus] = useState<AppStatus|null>(null);
  const [loadCount, setLoadCount] = useState(0);

  useEffect(() => {
    let cancelled = false;
    let timeout: number | undefined = undefined;

    async function getStatus() {
      try {
        const response : AppStatus = await (await fetch("/api/status.json")).json();

        if(!cancelled)
        {
          setAppStatus(response);
        }
      }
      catch(ex) {
        console.log(ex);
      }
      // Cause the status info to reload
      timeout = window.setTimeout(() => {
        setLoadCount(n => n+1);
      }, 10_000);
    };

    getStatus();
    return () => {
        cancelled = true;
        window.clearTimeout(timeout);
    };
  }, [loadCount]);

  if(appStatus == null)
    return (<Spinner animation="grow" variant="warning"  style={{ width: "8rem", height: "8rem" }} />);

  if(appStatus.apMode)
    // Limited UI for connecting to WiFi
    return (<HomePage {...appStatus} />);

  const router = createHashRouter(
    createRoutesFromElements(
      <Route path="/" element={<App/>}
      errorElement={<ErrorPage/>}>
      <Route index element={<HomePage {...appStatus}/>} />
      <Route path="/control" element={<ControlPage />} />
      <Route path="/setup" element={<SetupPage />} />
    </Route>
    ));
    
  return (<RouterProvider router={router} />);
}

const root = ReactDOM.createRoot(
  document.getElementById('root') as HTMLElement
);
root.render(
  <React.StrictMode>
    <AppModeSwitcher />
  </React.StrictMode>
);

