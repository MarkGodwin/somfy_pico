
import logo from './logo.svg';
import pico from './pico.svg';
import './Navbar.css';
import { Navbar, Nav, Container } from 'react-bootstrap';
import { LinkContainer } from 'react-router-bootstrap';

export function NavBar() {
  return (

      <Navbar className="navbar-expand-lg bg-light">
        <Container>
          <Navbar.Brand className="mb-0 h1">
            <img src={pico} alt="Pico" height={35} className="d-inline-block me-1" />
            <img src={logo} alt="Somfy" height={25} className="d-inline-block" />
          </Navbar.Brand>
          <Navbar.Toggle aria-controls="navbarNav" />
          <Navbar.Collapse id="navbarNav">
            <Nav>
              <LinkContainer to="/">
                <Nav.Link>
                    Home
                </Nav.Link>
              </LinkContainer>
              <LinkContainer to="/control">
                <Nav.Link>
                  Control Blinds
                </Nav.Link>
              </LinkContainer>
              <LinkContainer to="/setup">
                <Nav.Link>
                  Network Setup
                </Nav.Link>
              </LinkContainer>
            </Nav>
          </Navbar.Collapse>
        </Container>
      </Navbar>
);
}
