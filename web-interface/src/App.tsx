import React, { Component } from 'react';
import logo from './logo.svg';
import './App.css';
import { NavBar } from './Navbar';
import { Outlet } from 'react-router-dom';

export class App extends Component {

  render(): React.ReactNode {
    return (

      <div className="container">
        <NavBar />
        <Outlet />

      </div>  );
  }
}

