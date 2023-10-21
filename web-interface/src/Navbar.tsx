
import logo from './logo.svg';
import './Navbar.css';
import { NavLink } from 'react-router-dom';

export function NavBar() {
  return (

      <nav className="navbar navbar-expand-lg bg-light">
        <div className="container">
          <span className="navbar-brand mb-0 h1">
            <img src={logo} alt="Somfy" height={25} className="d-inline-block align-text-bottom" />
            Blind Controller
          </span>
          <button className="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarNav" aria-controls="navbarNav" aria-expanded="false" aria-label="Toggle navigation">
            <span className="navbar-toggler-icon"></span>
          </button>
          <div className="collapse navbar-collapse" id="navbarNav">
            <ul className="navbar-nav">
              <li className="nav-item">
                <NavLink className="nav-link" aria-current="page" to="/">Control Blinds</NavLink>
              </li>
              <li className="nav-item">
                <NavLink className="nav-link" to="/config">Configure</NavLink>
              </li>
              <li className="nav-item">
                <NavLink className="nav-link" to="/setup">Network Setup</NavLink>
              </li>
             </ul>
          </div>
        </div>
      </nav>
);
}
