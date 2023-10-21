import { useState, useMemo, ReactNode, PropsWithChildren, MouseEventHandler } from 'react';
import { createPortal } from 'react-dom';

import { ToasterContext } from './toaster';
import './toasterOutlet.css';
import { title } from 'process';

let toastIndex = 0;


type ToastProps = PropsWithChildren<{
    key: number;
    title: ReactNode;
    close: MouseEventHandler<HTMLButtonElement>;
}>;

const Toast = (props: ToastProps) => {
  setTimeout(props.close, 5000);

  return (
    <div className="toast show">
      <div className="toast-header">
        <svg
            className="bd-placeholder-img rounded me-2"
            width="20"
            height="20"
            xmlns="http://www.w3.org/2000/svg"
            aria-hidden="true"
            preserveAspectRatio="xMidYMid slice"
            focusable="false"
            >
            <rect width="100%" height="100%" fill="#ff3a10"></rect>
            </svg>
        <strong className="me-auto">{props.title}</strong>
        <button type="button" className="btn-close" onClick={props.close} aria-label="Close"></button>
      </div>
      <div className="toast-body">{props.children}</div>
    </div>
  );
};


interface ToastEntry {
    id: number;
    title: ReactNode;
    content: ReactNode;
};

export const ToasterOutlet = (props: { children?: ReactNode }) => {
    const [toasts, setToasts] = useState<ToastEntry[]>([]);

    const open = (title: ReactNode, content: ReactNode) =>
        setToasts((currentToasts) => [
        ...currentToasts,
        { id: toastIndex++, title, content },
        ]);
        
    const close = (id: number) =>
        setToasts((currentToasts) =>
        currentToasts.filter((toast) => toast.id !== id)
        );
    const contextValue = useMemo(() => ({ open }), []);

    return (
        <ToasterContext.Provider value={contextValue}>
            <div aria-live="polite" aria-atomic="true" className="toast-target">
                {props.children}
                <div className="toast-list">
                    {toasts.map((toast) => (
                        <Toast key={toast.id} title={toast.title} close={() => close(toast.id)}>
                        {toast.content}
                        </Toast>
                    ))}
                </div>            
            </div>

        </ToasterContext.Provider>
    );
};