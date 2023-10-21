import './HomePage.css';
import { Col, Container, ListGroup, Row } from "react-bootstrap";
import { WifiSetup } from './WifiSetup';
import logo from './logo.svg';


export function HomePage(props: {apMode: boolean, mqttConnected: boolean} ) {

  return (
    <>
      <div className="p-5 mt-4 mb-4 bg-body-tertiary rounded-3">
        <Container className="py-5">
          <h1 className="display-5 fw-bold">Mark's Unofficial <img src={logo} alt="Somfy" height={50} className="d-inline-block mb-2" />Blind Controller</h1>
          <p className="col-md-8 fs-5">Control your Somfy blinds and shutters, and integrate with Home Assistant via Mqtt to enable automation of your blinds. While there is an Web interface for blind control and configuration, and an HTTP API for blind control, day-to-day use should be via Home Assistant.</p>
          <p className="col-md-8 fs-5">It uses a cheap Raspberry Pi Pico-W module and an RFM69HCW module with a total cost of less than £10. This started as an experiment to convice the RFM69HCW module to produce Somfy format frames in packet mode.</p>
          <p className="col-md-8 fs-4">Credits:</p>
          <p className="col-md-8 fs-5">This was my attempt to fix the unreliable radio signal from the <a href="https://github.com/Nickduino/Pi-Somfy">Nickduino Pi-Somfy project</a>. It was built using protocol information documented by <a href="https://pushstack.wordpress.com/somfy-rts-protocol/">PushStack</a>.</p>
          <h2 className="col-md-8 fs-4">Controller status:</h2>

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
        </Container>
      </div>
    </>
  );
}