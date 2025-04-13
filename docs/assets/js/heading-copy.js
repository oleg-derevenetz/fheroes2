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
    const content = document.querySelector( "#main_content" ); // Updated selector

    if ( !content )
        return;

    // Define our themed icons
    const LINK_ICON = "🔗"; // Link icon
    const COPIED_ICON = "&nbsp;- Copied"; // message for confirmation
    content.querySelectorAll( "h2, h3, h4, h5, h6" ).forEach( ( heading ) => {
        // Create an ID if none exists
        if ( !heading.id ) {
            heading.id = heading.textContent.toLowerCase().replace( /[^a-z0-9]+/g, '-' );
        }

        // Create wrapper for original heading content
        const contentWrapper = document.createElement( "span" );
        while ( heading.firstChild ) {
            contentWrapper.appendChild( heading.firstChild );
        }

        // Create the anchor that wraps everything
        const anchor = document.createElement( "a" );
        anchor.href = `#${heading.id}`;
        anchor.className = "heading-link";
        anchor.style.textDecoration = "none";
        anchor.style.color = "inherit";

        // Add the content wrapper
        anchor.appendChild( contentWrapper );

        // Add the icon
        const icon = document.createElement( "span" );
        icon.className = "heading-anchor";
        icon.setAttribute( "aria-label", "Copy link to heading" );
        icon.innerHTML = LINK_ICON;
        anchor.appendChild( icon );

        // Clear the heading and add our new structure
        heading.appendChild( anchor );

        // Handle click events for the anchor icon only
        icon.addEventListener( "click", async ( e ) => {
            e.preventDefault();
            e.stopPropagation(); // Prevent the event from bubbling up to the anchor
            const url = `${window.location.origin}${window.location.pathname}#${heading.id}`;

            try {
                if ( navigator.clipboard && window.isSecureContext ) {
                    // For HTTPS or localhost
                    await navigator.clipboard.writeText( url );
                }
                else {
                    // Fallback for HTTP
                    const textArea = document.createElement( "textarea" );
                    textArea.value = url;
                    textArea.style.position = "fixed";
                    textArea.style.left = "-999999px";
                    textArea.style.top = "-999999px";
                    document.body.appendChild( textArea );
                    textArea.focus();
                    textArea.select();
                    try {
                        document.execCommand( 'copy' );
                    }
                    catch ( err ) {
                        console.error( 'Failed to copy:', err );
                    }
                    textArea.remove();
                }

                // Show copy confirmation with themed icon
                icon.innerHTML = COPIED_ICON;
                icon.style.transform = "scale(1.2)";

                setTimeout( () => {
                    icon.innerHTML = LINK_ICON;
                    icon.style.transform = "";
                }, 2000 );

                // Update URL without scrolling
                history.pushState( null, null, `#${heading.id}` );
            }
            catch ( err ) {
                console.error( 'Failed to copy:', err );
                icon.innerHTML = "❌";
                setTimeout( () => { icon.innerHTML = LINK_ICON; }, 2000 );
            }
        } );
    } );
} );
