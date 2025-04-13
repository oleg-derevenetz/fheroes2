/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

document.addEventListener( "DOMContentLoaded", function() {
    document.querySelectorAll( 'div.highlight' ).forEach( block => {
        const button = document.createElement( "button" );
        button.textContent = "Copy";
        button.className = "btn btn-primary copy-btn";
        button.addEventListener( "click", async () => {
            const code = block.querySelector( "pre" ).innerText;
            try {
                if ( navigator.clipboard && window.isSecureContext ) {
                    // For HTTPS or localhost
                    await navigator.clipboard.writeText( code );
                    button.textContent = "Copied!";
                }
                else {
                    // Fallback for HTTP
                    const textArea = document.createElement( "textarea" );
                    textArea.value = code;
                    textArea.style.position = "fixed";
                    textArea.style.left = "-999999px";
                    textArea.style.top = "-999999px";
                    document.body.appendChild( textArea );
                    textArea.focus();
                    textArea.select();
                    try {
                        document.execCommand( 'copy' );
                        button.textContent = "Copied!";
                    }
                    catch ( err ) {
                        button.textContent = "Failed to copy";
                    }
                    textArea.remove();
                }
            }
            catch ( err ) {
                button.textContent = "Failed to copy";
            }
            setTimeout( () => ( button.textContent = "Copy" ), 2000 );
        } );
        block.appendChild( button );
    } );
} );
