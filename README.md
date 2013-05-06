libVitaMTP and OpenCMA
======================

## What is this?

libVitaMTP is a library based off of libMTP that does low level USB 
communications with the Vita. It can read and recieve MTP commands 
that the Vita sends, which are a proprietary set of commands that is 
based on the MTP open standard.

OpenCMA is a frontend that allows the user to transfer games, saves, 
and media to and from the PlayStation Vita. It makes use of libVitaMTP 
to communicate with the device and other libraries to intrepet the data 
sent and recieved. OpenCMA is a command line tool that aims to be an 
open source replacement to Sony's offical Content Management Assistant.

## How do I use it?

To build OpenCMA from scratch, follow the directions in the INSTALL 
document. Binary releases will be distributed when the software is in 
a relevantly stable condition. Run "opencma -?" to see usage options.
For your conveience, the output for the help is also listed below

> usage: opencma paths [options]
>    paths
>        -p path     Path to photos
>        -v path     Path to videos
>        -m path     Path to music
>        -a path     Path to apps
>    options
>        -u path     Path to local URL mappings
>        -l level    logging level, number 1-4.
>                    1 = error, 2 = info, 3 = verbose, 4 = debug
>        -h          Show this help text
> 
> additional information:
> 
>    Any paths that are not specified will default to the current directory
>    that you are calling OpenCMA from. Please note that having larger
>    directories means that OpenCMA will run slower and use more memory.
>    This is because OpenCMA doesn't have an external database and builds
>    (and keeps) its database in memory. If you try to run OpenCMA with
>    paths that contains lots of files and directories it may quickly run
>    out of memory. Also beware that using the same path for multiple data
>    types (photos and videos, for example) is undefined behavior. It can
>    result in files not showing up without a manual database refresh
>    (CTRL+Z). Modifying the directory as OpenCMA is running may also
>    result in the same behavior.
> 
>    URL mappings allow you to redirect Vita's URL download requests to
>    some file locally. This can be used to, for example, change the file
>    for firmware upgrading when you choose to update the Vita via USB. The
>    Vita may request http://example.com/PSP2UPDAT.PUP and if you use the
>    optoin '-u /path/to/fw' then OpenCMA will send
>    /path/to/fw/PSP2UPDAT.PUP to the Vita. You do NOT need to do this for
>    psp2-updatelist.xml to bypass the update prompt because that file is
>    built in to OpenCMA for your convenience. If you do wish to  send a
>    custom psp2-updatelist.xml, you can.
> 
>    There are four logging levels that you can select with the '-l'
>    option. '-l 1' is the default and will only show critical error
>    messages. '-l 2' will allow you to see more of the behind-the-scenes
>    process such as what file is being sent and so on. '-l 3' will, in
>    addition, display advanced information like what event the Vita is
>    sending over, and is for the curious minded. '-l 4' will log
>    EVERYTHING including the raw USB traffic to and from the device.
>    PLEASE use this option when you are filing a bug report and attach the
>    output so the issue can be resolved quickly. Please note that more
>    logging means OpenCMA will run slower.;
> 
> additional information:
> 
>    Any paths that are not specified will default to the current directory
>    that you are calling OpenCMA from. Please note that having larger
>    directories means that OpenCMA will run slower and use more memory.
>    This is because OpenCMA doesn't have an external database and builds
>    (and keeps) its database in memory. If you try to run OpenCMA with
>    paths that contains lots of files and directories it may quickly run
>    out of memory. Also beware that using the same path for multiple data
>    types (photos and videos, for example) is undefined behavior. It can
>    result in files not showing up without a manual database refresh
>    (CTRL+Z). Modifying the directory as OpenCMA is running may also
>    result in the same behavior.
> 
>    URL mappings allow you to redirect Vita's URL download requests to
>    some file locally. This can be used to, for example, change the file
>    for firmware upgrading when you choose to update the Vita via USB. The
>    Vita may request http://example.com/PSP2UPDAT.PUP and if you use the
>    optoin '-u /path/to/fw' then OpenCMA will send
>    /path/to/fw/PSP2UPDAT.PUP to the Vita. You do NOT need to do this for
>    psp2-updatelist.xml to bypass the update prompt because that file is
>    built in to OpenCMA for your convenience. If you do wish to  send a
>    custom psp2-updatelist.xml, you can.
> 
>    There are four logging levels that you can select with the '-l'
>    option. '-l 1' is the default and will only show critical error
>    messages. '-l 2' will allow you to see more of the behind-the-scenes
>    process such as what file is being sent and so on. '-l 3' will, in
>    addition, display advanced information like what event the Vita is
>    sending over, and is for the curious minded. '-l 4' will log
>    EVERYTHING including the raw USB traffic to and from the device.
>    PLEASE use this option when you are filing a bug report and attach the
>    output so the issue can be resolved quickly. Please note that more
>    logging means OpenCMA will run slower.

## What is working?

As of the writing of this, the sending of Vita/PSX/PSP/PSM applications to 
and from the device works. Sending/recieving of PSP saves also work. Backups 
and restoring backups also work. However, edge cases have not been tested, 
and if you run into errors, please report it to the GitHub issues page.

## What needs work?

The major things that are not done yet are 1) media transfers (videos, music, 
and photos), 2) edge cases like "what happens if a file is deleted on the 
computer while it is being transfered?" and 3) testing OpenCMA on multiple 
configurations, hosts, and computers. For the last two, the help of beta 
testers is requested.

## How do I test?

Issues Page: https://github.com/yifanlu/VitaMTP/issues

First try to compile it using the directions in INSTALL, if the process fails 
post the output into the GitHub issues page. Once it is compiled and installed, 
run "opencma --help" to see how to use it. If you run into a problem, run OpenCMA 
again with debugging options to get a more verbose output. "opencma -l 4" will 
show more information about what OpenCMA is doing. "opencma -l 6" will print all 
the USB packets sent and recieved. "opencma -l 9" will print every USB event and 
other debugging information. For most problems, log the output of "opencma -l 6" 
and attach the log with your error report describing the problem you ran into and 
what you were trying to do. Also include what operating system you are running and 
any other relevant information.

### Why OpenCMA? Isn't there another project under the same name?

The "OpenCMA" you may have heard of is just a patch to prevent the offical CMA 
from communicating with Sony (and forcing the Vita to update). It is not at all 
"open." This OpenCMA is truly an open source re-implementation of CMA and deserves 
the name more. If you have a better name, feel free to suggest it. Please don't give 
me any names that has number(s) in it though. I will never contribute to a project 
named "0penCMA" or something like that.

## Credits?

[Yifan Lu](http://yifan.lu/) has been responsible for this abomination of code
dridri also takes some blame for his help in figuring out many structures and codes
