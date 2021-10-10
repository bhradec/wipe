# Wipe

A simple file and device secure erasure utility. For available arguments
and options use `wipe --help`.

## Example of use

`wipe file.txt --method zero --passes 1 --verbose`

## Overwrite methods

Available overwrite methods are:

<table>
    <tr>
        <th>Method</th>
        <th>Decription</th>
    </tr>
    <tr>
        <td>zero</td>
        <td>Overwrite with zeroes (0x00000000).</td>
    </tr>
    <tr>
        <td>one</td>
        <td>Overwrite with ones (0x11111111).</td>
    </tr>
    <tr>
        <td>rand</td>
        <td>Overwrite with random data.</td>
    </tr>
</table>

## Example of use

### Checking file data before wiping

File data before wiping can be checked using a hex dump utility, for example `xxd`.
before wiping.

Beginning of the file:
![Beginning of the file before wiping](/res/xxd_1.png)

End of the file:
![Beginning of the file before wiping](/res/xxd_2.png)

### Finding the disk blocks that contain the file data

Info about the disk blocks containing the file data is stored in the corresponding inode of the filesystem the file is stored on. Info about the filesystem path and inode
can be retrieved using the `filedata` utility availabile at <https://github.com/bhradec/filedata>.

Output of the `filedata` utility:
![Output of the filedata utility](/res/filedata.png)

Blocks from the inode can be retrieved using `istat` utility from the `sleuthkit`.

Blocks are listed as "Direct blocks" in the output of the `istat`:
![Output of the istat utility](/res/istat.png)

### Disk block content before wiping

Content of the blocks can be retrieved using `blkcat` utility fro the `sleuthkit`.

Retrieveing 2 consequential blocks using the `blkcat` utility:
![Retrieveing 2 consequential blocks using the blkcat utility](/res/blkcat_before_1.png)

End of the second block:
![End of the second block](/res/blkcat_before_2.png)

### Wiping the file

![Wiping the file](/res/wipe.png)

### Disk block content after wiping

Retrieveing 2 consequential blocks using the `blkcat` utility after wiping the file:
![Retrieveing 2 consequential blocks using the blkcat utility after wiping](/res/blkcat_after_1.png)

End of the second block after wiping the file:
![End of the second block after wiping](/res/blkcat_after_2.png)

## Important notice

The `wipe` utility does not always work on the SSD drives because of wear leveling and other things. For SSD drives, wiping the whole drive is a safer solution.
