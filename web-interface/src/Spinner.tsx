

import React from 'react';

export function Spinner(props: {loading: boolean}) {

    return (
            <div className="spinner-border float-end" role="status" hidden={!props.loading}>
                <span className="visually-hidden">Loading...</span>
            </div>
        );
}