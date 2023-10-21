import React from 'react';
import ReactDOM from 'react-dom/client';
import 'bootstrap/dist/css/bootstrap.css';

import {
  createBrowserRouter,
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

const router = createBrowserRouter(
  createRoutesFromElements(
    <Route path="/" element={<App/>}
    errorElement={<ErrorPage/>}>
    <Route index element={<HomePage/>} />
    <Route path="/control" element={<ControlPage />} />
    <Route path="/setup" element={<SetupPage />} />
  </Route>
  ));

const root = ReactDOM.createRoot(
  document.getElementById('root') as HTMLElement
);
root.render(
  <React.StrictMode>
    <RouterProvider router={router} />
  </React.StrictMode>
);

