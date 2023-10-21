
export interface BlindConfig {
    id: number;
    name: string;
    position: number;
    openTime: number;
    closeTime: number;
    remoteId: number;
  };
  
export interface RemoteConfig {
    id : number;
    name: string;
    blinds: number[]
  };
  