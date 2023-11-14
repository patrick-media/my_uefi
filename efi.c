#include"efi.h"

EFI_GUID g_loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID; // guid for loaded image protocol - allows accessing image functions
EFI_GUID g_device_path_protocol_guid = EFI_DEVICE_PATH_PROTOCOL_GUID;

#define SCANCODE_ESC    0x17
#define SCANCODE_UP     0x01
#define SCANCODE_DOWN   0x02
#define SCANCODE_RIGHT  0x03
#define SCANCODE_LEFT   0x04

#define HLT() while( TRUE )

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout = NULL;
EFI_SIMPLE_TEXT_INPUT_PROTOCOL *cin = NULL;
EFI_BOOT_SERVICES *bs = NULL;
EFI_SYSTEM_TABLE *st = NULL;
EFI_HANDLE ih = NULL;

typedef struct {
    UINTN x;
    UINTN y;
} efi_cursor_data_t;
efi_cursor_data_t efi_cursor_data = { 0 };

UINTN strcmp( CHAR16 s1[], CHAR16 s2[] ) {
    UINTN i;
    for( i = 0; s1[ i ] == s2[ i ]; i++ ) {
        if( s1[ i ] == u'\0' ) return 0;
    }
    return s1[ i ] - s2[ i ];
}
UINTN strlen( CONST CHAR16 s[] ) {
    UINTN i = 0;
    while( s[ i ] ) i++;
    return i;
}
/*
UINTN atoi( CHAR16 s[], UINTN base ) {
    UINTN value = 0;
    if( base == 10 ) {
        for( UINTN i = 0; i < strlen( s ); i++ ) {
            value = value * 10 + 
        }
    } else if( base == 16 ) {
        //
    } else {
        printf( u"Error: atoi() failed. Halting.\r\n" );
        HLT();
    }
}
*/

EFI_INPUT_KEY efi_get_key() {
    EFI_EVENT events[ 1 ];
    EFI_INPUT_KEY key;
    key.ScanCode = 0;
    key.UnicodeChar = u'\0';
    events[ 0 ] = cin->WaitForKey;
    UINTN index = 0;
    bs->WaitForEvent( 1, events, &index );
    if( index == 0 ) cin->ReadKeyStroke( cin, &key );
    return key;
}

void _print_num( UINT32 num, UINT8 base ) {
    const char digits[ 17 ] = "0123456789ABCDEF";
    CHAR16 buffer[ 11 ] = { 0 };
    UINTN i = 0;
    do {
        buffer[ i++ ] = digits[ num % base ];
        num /= base;
    } while( num > 0 );
    buffer[ i-- ] = u'\0';
    for( UINTN j = 0; j < i; j++, i-- ) {
        UINTN temp = buffer[ i ];
        buffer[ i ] = buffer[ j ];
        buffer[ j ] = temp;
    }
    cout->OutputString( cout, buffer );
    efi_cursor_data.x += strlen( buffer );
}

BOOLEAN printf( CHAR16 *fmt, ... ) {
    BOOLEAN result = TRUE;
    CHAR16 charstr[ 2 ] = { 0 };
    charstr[ 1 ] = u'\0';
    va_list args;
    va_start( args, fmt );
    for( UINTN i = 0; fmt[ i ] != u'\0'; i++ ) {
        if( fmt[ i ] == u'%' ) {
            i++;
            switch( fmt[ i ] ) {
                case u'c': {
                        CHAR16 c[ 2 ] = { fmt[ i ], u'\0' };
                        cout->OutputString( cout, c );
                        efi_cursor_data.x++;
                    }
                    break;
                case u's': {
                        CHAR16 *string = va_arg( args, CHAR16* );
                        cout->OutputString( cout, string );
                        efi_cursor_data.x += strlen( string );
                    }
                    break;
                case u'd': {
                        INT32 number = va_arg( args, INT32 );
                        _print_num( number, 10 );
                    }
                    break;
                case u'x': {
                        UINTN number = va_arg( args, UINTN );
                        _print_num( number, 16 );
                    }
                    break;
                default:
                    cout->OutputString( cout, u"Invalid format specifier: %" );
                    charstr[ 0 ] = fmt[ i ];
                    cout->OutputString( cout, charstr );
                    cout->OutputString( cout, u"\r\n" );
                    efi_cursor_data.x = 0;
                    efi_cursor_data.y++;
                    result = FALSE;
                    goto end;
                    break;
            }
        } else {
            charstr[ 0 ] = fmt[ i ];
            cout->OutputString( cout, charstr );
            switch( fmt[ i ] ) {
                case u'\n': {
                        efi_cursor_data.y++;
                    }
                    break;
                case u'\r': {
                        efi_cursor_data.x = 0;
                    }
                    break;
                default: {
                        efi_cursor_data.x++;
                    }
                    break;
            }
        }
    }
    end:
    va_end( args );
    return result;
}

void set_cursor_position( UINTN cols, UINTN rows ) {
    cout->SetCursorPosition( cout, cols, rows );
    efi_cursor_data.x = cols;
    efi_cursor_data.y = rows;
}
void set_attribute( UINTN fg, UINTN bg ) {
    cout->SetAttribute( cout, EFI_TEXT_ATTR( fg, bg ) );
}
void clear_screen( void ) {
    cout->ClearScreen( cout );
}

UINTN sh_help( UINT32 argc, CHAR16 argv[ 8 ][ 32 ] ) {
    ( void )argc, ( void )argv;
    printf( u"help not implemented yet :(\r\n" );
    return EFI_SUCCESS;
}
UINTN sh_shutdown( UINT32 argc, CHAR16 argv[ 8 ][ 32 ] ) {
    ( void )argv;
    if( argc > 1 ) {
        printf( u"Invalid number of arguments supplied: %d\r\n", argc );
        return EFI_SUCCESS;
    }
    printf( u"Shutting down...\r\n" );
    st->RuntimeServices->ResetSystem( EfiResetShutdown, EFI_SUCCESS, 0, NULL );
    return EFI_SUCCESS; // should never execute
}
UINTN sh_reboot( UINT32 argc, CHAR16 argv[ 8 ][ 32 ] ) {
    ( void )argv;
    if( argc > 1 ) {
        printf( u"Invalid number of arguments supplied: %d\r\n", argc );
        return EFI_SUCCESS;
    }
    printf( u"Rebooting...\r\n" );
    st->RuntimeServices->ResetSystem( EfiResetWarm, EFI_SUCCESS, 0, NULL );
    return EFI_SUCCESS; // should never execute
}
UINTN sh_echo( UINT32 argc, CHAR16 argv[ 8 ][ 32 ] ) {
    if( argc < 2 ) {
        printf( u"Invalid number of arguments supplied: %d\r\n", argc );
        return EFI_SUCCESS;
    }

    UINTN i = 1;
    while( i < argc ) {
        printf( u"%s ", argv[ i++ ] );
    }
    return EFI_SUCCESS;
}

UINTN menu_textmode( void ) {
    UINTN cols, rows;
    INT32 selected = 0;
    set_cursor_position( 0, 0 );
    set_attribute( EFI_WHITE, EFI_BLUE );
    clear_screen();

    set_attribute( EFI_WHITE, EFI_MAGENTA );
    printf( u"Set Text Mode (press ESC to exit)\r\n\r\n" );
    set_attribute( EFI_WHITE, EFI_BLUE );

    cout->QueryMode( cout, cout->Mode->Mode, &cols, &rows );
    printf( u"Currently: Mode %d %dx%d\r\n", cout->Mode->Mode, cols, rows );
    printf( u"Available Modes:\r\n" );

    printf( u"┌──────────────────────┐\r\n" );
    BOOLEAN redraw = FALSE;
    while( TRUE ) {
        if( redraw ) {
            redraw = FALSE;
            set_cursor_position( 0, 0 );
            set_attribute( EFI_WHITE, EFI_BLUE );
            clear_screen();

            set_attribute( EFI_WHITE, EFI_MAGENTA );
            printf( u"Set Text Mode (press ESC to exit)\r\n\r\n" );
            set_attribute( EFI_WHITE, EFI_BLUE );

            cout->QueryMode( cout, cout->Mode->Mode, &cols, &rows );
            printf( u"Currently: Mode %d %dx%d\r\n", cout->Mode->Mode, cols, rows );
            printf( u"Available Modes:\r\n" );

            printf( u"┌──────────────────────┐\r\n" );
        } else {
            cout->QueryMode( cout, cout->Mode->Mode, &cols, &rows );
            set_cursor_position( 16, 2 );
            printf( u"%d %dx%d\r\n", cout->Mode->Mode, cols, rows );
        }

        set_cursor_position( 0, 5 );
        for( INT32 i = 0; i < cout->Mode->MaxMode; i++ ) {
            cout->QueryMode( cout, i, &cols, &rows );
            set_attribute( EFI_WHITE, EFI_BLUE );
            printf( u"│" );
            if( i == selected ) set_attribute( EFI_YELLOW, EFI_CYAN );
            printf( u"   Mode %d: %dx%d", i, cols, rows );
            while( efi_cursor_data.x < 23 ) {
                printf( u" " );
            }
            set_attribute( EFI_WHITE, EFI_BLUE );
            printf( u"│\r\n" );
        }
        printf( u"└──────────────────────┘\r\n" );
        EFI_INPUT_KEY key = efi_get_key();

        BOOLEAN change_mode = FALSE;
        switch( key.ScanCode ) {
            case SCANCODE_ESC: {
                    return EFI_SUCCESS;
                }
                break;
            case SCANCODE_UP: {
                    if( selected > 0 ) selected--;
                }
                break;
            case SCANCODE_DOWN: {
                    if( selected < cout->Mode->MaxMode - 1 ) selected++;
                }
                break;
            default: {
                    if( key.UnicodeChar == u'\r' ) {
                        change_mode = TRUE;
                        redraw = TRUE;
                        break; // skip rest of statememt (error stuff)
                    }
                    set_cursor_position( 35, 0 );
                    printf( u"Error: default case handled. Halting.\r\n" );
                    HLT();
                }
                break;
        }
        if( change_mode ) {
            EFI_STATUS status = cout->SetMode( cout, selected );
            if( EFI_ERROR( status ) ) {
                if( status == ( EFI_STATUS )EFI_DEVICE_ERROR ) {
                    set_attribute( EFI_WHITE, EFI_RED );
                    set_cursor_position( 35, 0 );
                    printf( u"Device error. Halting.\r\n" );
                    HLT();
                }
                if( status == ( EFI_STATUS )EFI_UNSUPPORTED ) {
                    set_attribute( EFI_WHITE, EFI_RED );
                    set_cursor_position( 35, 0 );
                    printf( u"Error: mode is invalid. Halting.\r\n" );
                    HLT();
                }
            }
        }
    }
}
UINTN menu_gfxmode( void ) {
    UINTN cols, rows;
    INT32 selected = 0;
    set_cursor_position( 0, 0 );
    set_attribute( EFI_WHITE, EFI_BLUE );
    clear_screen();

    set_attribute( EFI_WHITE, EFI_MAGENTA );
    printf( u"Set Text Mode (press ESC to exit)\r\n\r\n" );
    set_attribute( EFI_WHITE, EFI_BLUE );

    cout->QueryMode( cout, cout->Mode->Mode, &cols, &rows ); // TODO this isn't gop
    printf( u"Currently: Mode %d %dx%d\r\n", cout->Mode->Mode, cols, rows );
    printf( u"Available Modes:\r\n" );

    printf( u"┌──────────────────────┐\r\n" );
    BOOLEAN redraw = FALSE;
    while( TRUE ) {
        if( redraw ) {
            redraw = FALSE;
            set_cursor_position( 0, 0 );
            set_attribute( EFI_WHITE, EFI_BLUE );
            clear_screen();

            set_attribute( EFI_WHITE, EFI_MAGENTA );
            printf( u"Set Text Mode (press ESC to exit)\r\n\r\n" );
            set_attribute( EFI_WHITE, EFI_BLUE );

            cout->QueryMode( cout, cout->Mode->Mode, &cols, &rows );
            printf( u"Currently: Mode %d %dx%d\r\n", cout->Mode->Mode, cols, rows );
            printf( u"Available Modes:\r\n" );

            printf( u"┌──────────────────────┐\r\n" );
        } else {
            cout->QueryMode( cout, cout->Mode->Mode, &cols, &rows );
            set_cursor_position( 16, 2 );
            printf( u"%d %dx%d\r\n", cout->Mode->Mode, cols, rows );
        }

        set_cursor_position( 0, 5 );
        for( INT32 i = 0; i < cout->Mode->MaxMode; i++ ) {
            cout->QueryMode( cout, i, &cols, &rows );
            set_attribute( EFI_WHITE, EFI_BLUE );
            printf( u"│" );
            if( i == selected ) set_attribute( EFI_YELLOW, EFI_CYAN );
            printf( u"   Mode %d: %dx%d", i, cols, rows );
            while( efi_cursor_data.x < 23 ) {
                printf( u" " );
            }
            set_attribute( EFI_WHITE, EFI_BLUE );
            printf( u"│\r\n" );
        }
        printf( u"└──────────────────────┘\r\n" );
        EFI_INPUT_KEY key = efi_get_key();

        BOOLEAN change_mode = FALSE;
        switch( key.ScanCode ) {
            case SCANCODE_ESC: {
                    return EFI_SUCCESS;
                }
                break;
            case SCANCODE_UP: {
                    if( selected > 0 ) selected--;
                }
                break;
            case SCANCODE_DOWN: {
                    if( selected < cout->Mode->MaxMode - 1 ) selected++;
                }
                break;
            default: {
                    if( key.UnicodeChar == u'\r' ) {
                        change_mode = TRUE;
                        redraw = TRUE;
                        break; // skip rest of statememt (error stuff)
                    }
                    set_cursor_position( 35, 0 );
                    printf( u"Error: default case handled. Halting.\r\n" );
                    HLT();
                }
                break;
        }
        if( change_mode ) {
            EFI_STATUS status = cout->SetMode( cout, selected );
            if( EFI_ERROR( status ) ) {
                if( status == ( EFI_STATUS )EFI_DEVICE_ERROR ) {
                    set_attribute( EFI_WHITE, EFI_RED );
                    set_cursor_position( 35, 0 );
                    printf( u"Device error. Halting.\r\n" );
                    HLT();
                }
                if( status == ( EFI_STATUS )EFI_UNSUPPORTED ) {
                    set_attribute( EFI_WHITE, EFI_RED );
                    set_cursor_position( 35, 0 );
                    printf( u"Error: mode is invalid. Halting.\r\n" );
                    HLT();
                }
            }
        }
    }
}
UINTN menu_shell( void ) {
    set_attribute( EFI_WHITE, EFI_BLACK );
    clear_screen();
    set_cursor_position( 0, 0 );
    printf( u"> " );

    CHAR16 buf[ 256 ] = { 0 };
    UINTN bufp = 0;

    #define NUM_CMDS 4
    typedef struct {
        CHAR16 cmdstr[ 16 ];
        UINTN ( *cmdfunc )( UINT32 argc, CHAR16 argv[ 8 ][ 32 ] );
    } cmds_list_t;
    cmds_list_t cmds_list[ NUM_CMDS ] = {
        { u"help", sh_help },
        { u"shutdown", sh_shutdown },
        { u"reboot", sh_reboot },
        { u"echo", sh_echo }
    };

    UINTN cursor_y = 0;
    while( TRUE ) {
        EFI_INPUT_KEY key = efi_get_key();
        CHAR16 c[ 2 ] = { key.UnicodeChar, u'\0' };
        printf( u"%s", c );
        if( key.UnicodeChar == u'\r' ) {
            cursor_y++;
            set_cursor_position( 0, cursor_y );

            CHAR16 l_argv[ 8 ][ 32 ] = { 0 }; // local argv container
            UINT32 l_argc = 0; // local argc counter
            BOOLEAN l_found = FALSE; // searched entire stirng y/n
            UINTN bi = 0; // buf position
            while( !l_found ) {
                UINTN k = 0; // argv entry string position
                while( buf[ bi ] != u' ' ) { // stop this arg at space
                    if( buf[ bi ] == u'\0' ) {
                        l_found = TRUE; // stop at the end of the string
                        break;
                    }
                    l_argv[ l_argc ][ k++ ] = buf[ bi++ ]; // add character to argv
                }
                l_argv[ l_argc ][ k ] = u'\0'; // null character
                l_argc++; // next argument
                bi++; // next string position - hit space
            }

            /*
            cmds to add:
            - pb (program bytes application)
            - help (add help info)
            - uname (show system info)
            */
            if( strcmp( buf, u"exit" ) == 0 ) { // special case - check this before cmds list
                return EFI_SUCCESS;
            }
            BOOLEAN cmd_found = FALSE; // print cmd error string y/n
            for( UINTN i = 0; i < NUM_CMDS; i++ ) { // loop through valid commands
                if( strcmp( l_argv[ 0 ], cmds_list[ i ].cmdstr ) == 0 ) {
                    cmd_found = TRUE; // don't print cmd not found string
                    UINTN status = cmds_list[ i ].cmdfunc( l_argc, l_argv ); // call associated function
                    if( EFI_ERROR( status ) ) { // error check status return
                        printf( u"Program '%s' exited with a return value of %d.\r\n", status );
                    }
                }
            }
            if( !cmd_found ) printf( u"Command not recognized. Type 'help' for help.\r\n" ); // cmd not found
            cursor_y++;
            set_cursor_position( 0, cursor_y );
            printf( u"> " );
            for( UINTN i = 0; i < 256; i++ ) {
                buf[ i ] = u'\0'; // reset full buf string
            }
            bufp = 0;
        } else {
            // backspace
            if( key.UnicodeChar == 8 ) {
                if( bufp > 0 ) {
                    bufp--;
                    buf[ bufp ] = u'\0';
                } else {
                    set_cursor_position( 2, cursor_y );
                }
            } else {
                buf[ bufp++ ] = c[ 0 ];
            }
        }
    }
    return EFI_SUCCESS;
}
UINTN menu_exit( void ) {
    set_attribute( EFI_WHITE, EFI_BLUE );
    printf( u"\r\nPress any key to shut down. Press ESC to cancel\r\n" );

    EFI_INPUT_KEY key = efi_get_key();
    if( key.ScanCode == SCANCODE_ESC ) return EFI_SUCCESS;

    st->RuntimeServices->ResetSystem( EfiResetShutdown, EFI_SUCCESS, 0, NULL );
    return EFI_SUCCESS; // should never execute
}

EFI_STATUS DevicePathLength( CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath, UINTN *Length ) {
    UINTN length_total = 0;
    UINT16 length_current = 0;
    EFI_DEVICE_PATH_PROTOCOL *CurrentNode = NULL;
    if( !DevicePath || !Length ) return EFI_INVALID_PARAMETER;
    *Length = 0;
    for(    CurrentNode = DevicePath;
            CurrentNode->Type != 0x7F, CurrentNode->SubType != 0xFF;
            CurrentNode = ( EFI_DEVICE_PATH_PROTOCOL* )( UINT8* )CurrentNode + *( ( UINT16* )CurrentNode->Length ) ) {
        length_current = *( ( UINT16* )CurrentNode->Length );
        if( length_current < sizeof( EFI_DEVICE_PATH_PROTOCOL ) ) return EFI_INVALID_PARAMETER;
        length_total += ( UINTN )length_current;
    }
    *Length = length_total;
    return EFI_SUCCESS;
}
EFI_STATUS LoadFile( CONST CHAR16 *Path, EFI_HANDLE ImageHandle ) {
    EFI_STATUS status;
    UINTN sz_path = 0;
    UINTN len_originaldp = 0;
    UINTN len_newdp = 0;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage = NULL;
    CONST EFI_DEVICE_PATH_PROTOCOL *OriginalDevicePath = NULL;
    EFI_DEVICE_PATH_PROTOCOL *NewDevicePath = NULL;
    EFI_DEVICE_PATH_PROTOCOL *NodeHelper = NULL;
    UINT8 *ByteHelper = NULL;
    EFI_HANDLE ImageToLoad = NULL;
    if( !Path ) return EFI_INVALID_PARAMETER;
    sz_path = strlen( Path );
    status = bs->HandleProtocol( ImageHandle, &g_loaded_image_guid, ( VOID** )&LoadedImage );
    if( EFI_ERROR( status ) ) {
        printf( u"Error: HandleProtocol with Loaded Image Protocol. 0x%x\r\n", status );
        HLT();
    }
    if( LoadedImage->DeviceHandle == NULL ) printf( u"dh is null\r\n" );
    status = bs->HandleProtocol( LoadedImage->DeviceHandle, &g_device_path_protocol_guid, ( VOID** )&OriginalDevicePath );
    if( EFI_ERROR( status ) ) {
        printf( u"Erorr: HandleProtocol with Device Path Protocol. 0x%x\r\n", status ); // error happens here: 0x2 invalid parameter
        HLT();
    }
    status = DevicePathLength( OriginalDevicePath, &len_originaldp );
    if( EFI_ERROR( status ) ) {
        printf( u"Error: DevicePathLength. 0x%x\r\n", status );
        HLT();
    }
    len_newdp = len_originaldp + sizeof( EFI_DEVICE_PATH_PROTOCOL ) + sz_path + sizeof( EFI_DEVICE_PATH_PROTOCOL );
    status = bs->AllocatePool( EfiLoaderData, len_newdp, ( VOID** )&NewDevicePath );
    if( EFI_ERROR( status ) ) {
        printf( u"Error: AllocatePool. 0x%x\r\n", status );
        HLT();
    }
    ByteHelper = ( UINT8* )NewDevicePath;
    bs->CopyMem( ByteHelper, ( VOID* )OriginalDevicePath, len_originaldp );
    ByteHelper += len_originaldp;
    NodeHelper = ( EFI_DEVICE_PATH_PROTOCOL* )ByteHelper;
    NodeHelper->Type = 0x04;
    NodeHelper->SubType = 0x04;
    *( ( UINT16* )NodeHelper->Length ) = ( UINT16 )( sizeof( EFI_DEVICE_PATH_PROTOCOL ) + sz_path );
    ByteHelper += sizeof( EFI_DEVICE_PATH_PROTOCOL );
    bs->CopyMem( ByteHelper, ( VOID* )Path, sz_path );
    ByteHelper += sz_path;
    NodeHelper = ( EFI_DEVICE_PATH_PROTOCOL* )ByteHelper;
    NodeHelper->Type = 0x7F;
    NodeHelper->SubType = 0xFF;
    *( ( UINT16* )NodeHelper->Length ) = sizeof( EFI_DEVICE_PATH_PROTOCOL );
    status = bs->LoadImage( FALSE, ImageHandle, NewDevicePath, NULL, 0, &ImageToLoad );
    if( EFI_ERROR( status ) ) {
        printf( u"Error: LoadImage on '%s'. 0x%x\r\n", Path, status );
        HLT();
    }
    status = bs->StartImage( ImageToLoad, NULL, NULL );
    if( EFI_ERROR( status ) ) {
        printf( u"Error: StartImage on '%s'. 0x%x\r\n", Path, status );
        HLT();
    }
    status = EFI_SUCCESS;
    if( NewDevicePath ) bs->FreePool( NewDevicePath );
    return status;
}

EFI_STATUS efi_main( EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable ) {
    cout = SystemTable->ConOut;
    cin = SystemTable->ConIn;
    bs = SystemTable->BootServices;
    st = SystemTable;
    ih = ImageHandle;

    //LoadFile( u"SHX64.EFI", ImageHandle );
    //HLT();

    set_attribute( EFI_WHITE, EFI_BLUE );
    clear_screen();

    UINTN selection = 0;
    #define NUM_MENUS 4
    UINTN ( *menufuncs[ NUM_MENUS ] )( void ) = { menu_textmode, menu_gfxmode, menu_shell, menu_exit };
    CHAR16 *menuopts[ NUM_MENUS ] = { u"Set Text Mode", u"Set Graphics Mode", u"Shell", u"Exit" };
    while( TRUE ) {
        clear_screen();
        set_cursor_position( 0, 0 );
        for( UINTN i = 0; i < NUM_MENUS; i++ ) {
            set_attribute( EFI_WHITE, EFI_BLUE );
            if( i == selection ) {
                set_attribute( EFI_YELLOW, EFI_BLUE );
            }
            printf( u"%s\r\n", menuopts[ i ] );
        }
        EFI_INPUT_KEY key = efi_get_key();
        switch( key.ScanCode ) {
            case SCANCODE_ESC: {
                    // TODO add escape function
                }
                break;
            case SCANCODE_UP: {
                    if( selection > 0 ) selection--;
                }
                break;
            case SCANCODE_DOWN: {
                    if( selection < NUM_MENUS-1 ) selection++;
                }
                break;
            case SCANCODE_RIGHT: {
                    // TODO add right function
                }
                break;
            case SCANCODE_LEFT: {
                    // TODO add left function
                }
                break;
            default: {
                    if( key.UnicodeChar == u'\r' ) {
                        UINTN status = menufuncs[ selection ]();
                        if( EFI_ERROR( status ) ) {
                            printf( u"Encountered an error: 0x%x\r\n", status );
                            while( TRUE );
                        }
                        set_attribute( EFI_WHITE, EFI_BLUE );
                    }
                }
                break;
        }
    }
    return EFI_SUCCESS;
}
