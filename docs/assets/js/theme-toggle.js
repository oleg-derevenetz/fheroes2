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

// Theme toggle functionality
document.addEventListener( 'DOMContentLoaded', () => {
    // Get the theme toggle button
    const themeToggle = document.getElementById( 'theme-toggle' );

    // Function to set the theme
    function setTheme( theme, saveToStorage = true )
    {
        document.documentElement.setAttribute( 'data-theme', theme );
        document.body.setAttribute( 'data-theme', theme );

        // Only save to localStorage if explicitly requested
        if ( saveToStorage ) {
            localStorage.setItem( 'theme', theme );
        }

        // Update button text
        if ( themeToggle ) {
            themeToggle.textContent = theme === 'dark' ? '☀️ Light Mode' : '🌙 Dark Mode';
        }
    }

    // Check for saved theme preference or use system preference
    const savedTheme = localStorage.getItem( 'theme' );
    if ( savedTheme ) {
        setTheme( savedTheme );
    }
    else {
        // Check system preference but don't save it to localStorage
        const prefersDark = window.matchMedia( '(prefers-color-scheme: dark)' ).matches;
        setTheme( prefersDark ? 'dark' : 'light', false );
    }

    // Add event listener to the toggle button
    if ( themeToggle ) {
        themeToggle.addEventListener( 'click', () => {
            const currentTheme = document.documentElement.getAttribute( 'data-theme' );
            setTheme( currentTheme === 'dark' ? 'light' : 'dark' );
        } );
    }

    // Listen for system theme changes
    window.matchMedia( '(prefers-color-scheme: dark)' ).addEventListener( 'change', ( e ) => {
        // Only apply system preference if user hasn't manually set a preference
        if ( !localStorage.getItem( 'theme' ) ) {
            setTheme( e.matches ? 'dark' : 'light', false );
        }
    } );
} );
