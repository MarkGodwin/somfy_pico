import React, { Component } from 'react';
import './App.css';
import { NavBar } from './Navbar';
import { Outlet } from 'react-router-dom';
import { ToasterOutlet } from './toasterOutlet';

export class App extends Component {

  render(): React.ReactNode {
    return (
      <>
      <NavBar />
      <div className="container">
        <ToasterOutlet>
          <Outlet />
        </ToasterOutlet>
      </div>
      </>);
  }
}

