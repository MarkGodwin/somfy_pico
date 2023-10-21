import { Component } from "react";
import { Form } from "react-router-dom";

export class ConfigPage extends Component {

  contact = {
    first: "Your",
    last: "Name",
    twitter: "your_handle",
    notes: "Some notes",
    favorite: true,
  };

  render(): React.ReactNode {

  
    return (
      <div id="contact">
  
        <div>
          <h1>
            {this.contact.first || this.contact.last ? (
              <>
                {this.contact.first} {this.contact.last}
              </>
            ) : (
              <i>No Name</i>
            )}{" "}
          </h1>
  
  
          <div>
            <Form method="get" action="edit">
              <button type="submit">Edit</button>
            </Form>
            <Form
              method="get"
              action="destroy"
            >
              <button type="submit">Delete</button>
            </Form>
          </div>
        </div>
      </div>
    );
            }
  }