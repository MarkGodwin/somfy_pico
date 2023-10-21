import { useEffect, useState } from 'react';
import { Form } from "react-router-dom";
import { WifiSetup } from './WifiSetup';
import { MqttSetup } from './MqttSetup';

interface WifiCreds {
  currentSsid: string,
  availableSsids: string[]
};

export function SetupPage() : JSX.Element {
    return (
      <div>
        <WifiSetup />
        <MqttSetup />
      </div>
    );
  }
