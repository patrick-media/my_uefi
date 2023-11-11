EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_LOADED_IMAGE_PROTOCOL *LoadedImage = NULL; // loaded image protocol "object"
CHAR16 path[ 10 ] = u"SHX64.EFI"; // file path - this is unused right now
EFI_STATUS status; // error checking

// note: using LocateProtocol because HandleProtocol and OpenProtocol seemed to have issues with the way I set them up and this seems to work virtually the same
status = SystemTable->BootServices->LocateProtocol( &loaded_image_guid, NULL, ( VOID** )&LoadedImage ); // load the protocol options into LoadedImage
if( EFI_ERROR( status ) ) {
    printf( u"Failed: LocateProtocol on EFI_LOADED_IMAGE_PROTOCOL. 0x%x\r\n", status );
    HLT();
}

EFI_GUID device_path_guid = EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_DEVICE_PATH_PROTOCOL *DevicePath = NULL; // originally retrieved device path

status = SystemTable->BootServices->LocateProtocol( &device_path_guid, NULL, ( VOID** )&DevicePath );
if( EFI_ERROR( status ) ) {
    printf( u"Failed: LocateProtocol on EFI_DEVICE_PATH_PROTOCOL. 0x%x\r\n", status );
    HLT();
}

UINTN dp_count = 0; // device path nodes count
BOOLEAN device_path_loop = TRUE; // perform loop y/n

/*
note: I think I understand pointers enough to have confidence with this?
but in reality I'm not sure if I should be referencing nodes by
(DevicePath + dp_count) as nodes have an arbitrary length.
*/
while( device_path_loop ) {
    if( ( DevicePath + dp_count )->Type == 0x7F && // check if type is 0x7F (end)
                ( DevicePath + dp_count )->SubType == 0xFF ) // check if subtype is 0xFF (end of entire path on QEMU)
        ) {
        device_path_loop = FALSE; // stop looping
    }
    // this part is my concern: I don't know if this is printing the actual node information or if it's printing the node data held beyond the header. full printed data is at the bottom
    printf( u"%d: %x %x %x %x\r\n", dp_count, ( DevicePath + dp_count )->Type, ( DevicePath + dp_count )->SubType, ( DevicePath + dp_count )->Length[ 0 ], ( DevicePath + dp_count )->Length[ 1 ] ); // print options
    dp_count++; // next device path node
}

EFI_DEVICE_PATH_PROTOCOL *NewDevicePath = NULL; // device path copy - sent into LoadImage later
status = SystemTable->BootServices->AllocatePool( EfiReservedMemoryType, ( dp_count )*sizeof( EFI_DEVICE_PATH_PROTOCOL ), ( VOID** )&NewDevicePath ); // EfiReservedMemoryType seems to work, others gave 0x2 invalid operand error
if( EFI_ERROR( status ) ) {
    printf( u"Failed: AllocatePool on EFI_DEVICE_PATH_PROTOCOL *NewDevicePath. 0x%x\r\n", status );
    HLT();
}

// copy the entire device path to a new variable
// is this not enough space because of the differing lengths of the nodes?
status = SystemTable->BootServices->CopyMem( NewDevicePath, DevicePath, ( dp_count+1 )*sizeof( EFI_DEVICE_PATH_PROTOCOL ) );
if( EFI_ERROR( status ) ) {
    printf( u"Failed: CopyMem on NewDevicePath (dest) and DevicePath (src) of size %d. 0x%x\r\n", dp_count, status );
    HLT();
}

EFI_FILE_PATH_NODE *new_path_node = NULL; // new node to add at the end
// this size should be enough, right? I'm not using more than this node type provides
status = SystemTable->BootServices->AllocatePool( EfiReservedMemoryType, sizeof( EFI_FILE_PATH_NODE ), ( VOID** )&new_path_node );
if( EFI_ERROR( status ) ) {
    printf( u"Failed: AllocatePool on EFI_FILE_PATH_NODE *new_path_node. 0x%x\r\n", status );
    HLT();
}

new_path_node->Hdr.Type = 4;
new_path_node->Hdr.SubType = 4;
new_path_node->Hdr.Length[ 0 ] = ( UINT8 )sizeof( *new_path_node ); // I think I'm doing this right?
new_path_node->Hdr.Length[ 1 ] = ( UINT8 )sizeof( *new_path_node ) >> 8; // not really sure
new_path_node->Path[ 0 ] = u'/'; // ignore all of this crap I was struggling with CopyMem and gave up. I tried no slash, forward slash / and backslash \ 
new_path_node->Path[ 1 ] = u'S';
new_path_node->Path[ 2 ] = u'H';
new_path_node->Path[ 3 ] = u'X';
new_path_node->Path[ 4 ] = u'6';
new_path_node->Path[ 5 ] = u'4';
new_path_node->Path[ 6 ] = u'.';
new_path_node->Path[ 7 ] = u'E';
new_path_node->Path[ 8 ] = u'F';
new_path_node->Path[ 9 ] = u'I';
new_path_node->Path[ 10 ] = u'\0';

printf( u"\r\n%s\r\n", &new_path_node->Path ); // debug to make sure the path was right - this is how I found I was using CopyMem wrong

// path end - trying to copy the end path node data into a temporary variable to plug it back in one node later.
EFI_DEVICE_PATH_PROTOCOL path_end = { 0 };
SystemTable->BootServices->CopyMem( &path_end, ( NewDevicePath + dp_count-1 ), sizeof( *( NewDevicePath + dp_count-1 ) ) ); // position 5 - extract end node data

SystemTable->BootServices->CopyMem( ( NewDevicePath + dp_count-1 ), new_path_node, sizeof( *new_path_node ) ); // position 5 - insert new node data

SystemTable->BootServices->CopyMem( ( NewDevicePath + dp_count ), &path_end, sizeof( *( NewDevicePath + dp_count ) ) ); // position 6 - insert end node data


printf( u"\r\n\r\n" );
device_path_loop = TRUE;
UINTN dp_i = 0;
// print the data again until the ending node
while( device_path_loop ) {
    if( ( NewDevicePath + dp_i )->Type == 0x7F && // check if type is 0x7F (end)
                ( NewDevicePath + dp_i )->SubType == 0xFF ) // check if subtype is 0x01 OR 0xFF (end)
        ) {
        device_path_loop = FALSE; // stop looping
    }
    // same as the one above
    printf( u"%d: %x %x %x %x\r\n", dp_i, ( NewDevicePath + dp_i )->Type, ( NewDevicePath + dp_i )->SubType, ( NewDevicePath + dp_i )->Length[ 0 ], ( NewDevicePath + dp_i )->Length[ 1 ] ); // print options
    dp_i++; // next device path node
}

EFI_HANDLE sh_handle = NULL; // handle for an image I want to load
printf( u"before load\r\n" ); // debug
status = SystemTable->BootServices->LoadImage( FALSE, ImageHandle, NewDevicePath, NULL, 0, &sh_handle ); // hanging here - "before load" prints, but the code in my other file doesn't get run and "after load" never shows up.
printf( u"after load\r\n" ); // debug
if( EFI_ERROR( status ) ) {
    printf( u"Failed: LoadImage. 0x%x\r\n", status );
    HLT();
}

printf( u"before start\r\n" ); // debug
status = SystemTable->BootServices->StartImage( sh_handle, NULL, NULL ); // never executed
if( EFI_ERROR( status ) ) {
    printf( u"Failed: StartImage. 0x%x\r\n", status );
    HLT();
}
printf( u"returned\r\n" ); // debug
HLT();
// beyond this is the rest of my efi_main function which works properly

/*

| FORMAT |
iteration #: type subtype length[0] length[1]

printed data: first attempted iteration of nodes
0: 4 7 14 0
1: C9 BD BB 7C
2: EB F8 34 4F
3: AA EA 3E E4
4: AF 65 16 A1
5: 7F FF 4 0

printed data: second attempted iteration of new nodes
0: 4 7 14 0
1: C9 BD B8 7C
2: EB F8 34 4F
3: AA EA 3E E4
4: AF 65 16 A1
5: 4 4 24 0      <-- this is my new node, it is correct but it may not be in the correct place
6: 7F FF 4 0

*/
