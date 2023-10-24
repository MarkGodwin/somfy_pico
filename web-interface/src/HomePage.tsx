import './HomePage.css';
import { Col, Container, ListGroup, Row } from "react-bootstrap";
import { WifiSetup } from './WifiSetup';
import logo from './logo.svg';
import pico from './pico.svg';


export function HomePage(props: {apMode: boolean, mqttConnected: boolean} ) {

  return (
    <>
      <div className="p-5 mt-4 mb-4 bg-body-tertiary rounded-3">
        <Container className="py-5">
          <h1 className="display-5 fw-bold">
            <img src={pico} alt="Pico" height={70} className="d-inline-block me-1" />
            <img src={logo} alt="Somfy" height={50} className="d-inline-block" /></h1>
          <h2 className="fs-4">Mark's Unofficial Somfy Blind Controller</h2>
          <p className="col-md-8 fs-5">Control your Somfy blinds and shutters, and integrate with Home Assistant via Mqtt to enable automation of your blinds.</p>
          <p className="col-md-8 fs-5">Use this web interface to configure the controller and add/remove blinds and remotes.</p>
          <p className="col-md-8 fs-5">While the Web interface does allow basic blind control, day-to-day use should be via Home Assistant through the MQTT integration.</p>
          <h2 className="col-md-8 fs-4 mt-4">Controller status:</h2>

          <Row>
            <Col className="col-md-12 col-lg-6">
          <ListGroup horizontal>
            <ListGroup.Item>
                <div className="fw-bold">WiFi Mode</div>
                <div>
                  {props.apMode?"In Access Point mode, functionality is limited. Enter your WiFi credentials below to connect the controller to your home network.":"Connected successfully to the WiFi network! Full controller functionality is available."}
              </div>
            </ListGroup.Item>
            <ListGroup.Item variant={props.apMode?"danger":"success"}>
              <div className="fw-bold" >{props.apMode?"Access Point":"Connected"}</div>
            </ListGroup.Item>
            </ListGroup>
            </Col>
            <Col className="col-md-12 col-lg-6">
            <ListGroup horizontal>
            <ListGroup.Item>
                <div className="fw-bold">Mqtt Status</div>
                <div>{props.mqttConnected?
                  "Mqtt connection established successfully! Home Assistant integration is enabled.":
                  "Mqtt connection error. Enter your Mqtt Server details on the Network Setup page."}</div>
            </ListGroup.Item>
            <ListGroup.Item variant={props.mqttConnected?"success":"danger"} >
              <div className="fw-bold" >{props.mqttConnected?"Connected":"Disconnected"}</div>
            </ListGroup.Item>
            </ListGroup>
          </Col>
          </Row>  
          {props.apMode ? <WifiSetup /> : null}

          <h2 className="col-md-8 fs-4 mt-5">Credits:</h2>
          <p>
            <ul className="fs-5">
              <li>Original concept from <a href="https://github.com/Nickduino/Pi-Somfy">Nickduino Pi-Somfy project</a>.</li>
              <li>Protocol information reverse engineered by <a href="https://pushstack.wordpress.com/somfy-rts-protocol/">PushStack</a>.</li>
            </ul>
          </p>
          <p>Copyright ©️ 2023 Mark Godwin.</p>
        </Container>
      </div>
    </>
  );
}