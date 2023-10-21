import { ReactNode, createContext, useContext } from 'react'


export const ToasterContext = createContext<{ open: (title: ReactNode, content: ReactNode) => void}>({
    open: function (title: ReactNode, content: ReactNode): void {
        throw new Error('Function not implemented.');
    }
});

export const useToaster = () => useContext(ToasterContext);
