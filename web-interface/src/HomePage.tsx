import { Component } from "react";
import { Form } from "react-router-dom";

export function HomePage() {
  
  return (
    <div>
      <div>
        <h1>
          Hello
        </h1>
        <p>
          Things to put here - status information. Is MQTT connected? Uptime? Etc.
        </p>
      </div>
    </div>
  );
}