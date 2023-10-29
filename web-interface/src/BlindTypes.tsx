
export interface BlindConfig {
    id: number;
    name: string;
    position: number;
    openTime: number;
    closeTime: number;
    remoteId: number;
    state: string;
  };
  
export interface RemoteConfig {
    id : number;
    name: string;
    blinds: number[];
    external: boolean;
  };
  