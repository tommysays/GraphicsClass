Joshua Lee
CMPSC 458

Programming Assignment 1

--- VIDEO GUIDE ----------
I begin by showing rotation around all axis (though I have a few scaling and translations in between by accident.)
Then I proceed to show translation. I do this by looking at some corner of my skybox and translating one way, then back, for each axis. This way, you have the illusion that we are moving around.
Then I shift our camera to the heightmap, so we can observe scaling.
I scale up and down in each direction.
Then I demonstrate my extra-credit portion, where I cycle through different color schemes for the heightmap.

--- USAGE GUIDE ----------
Enter a valid image filename in the prompt.
Rotate your camera holding mouse left button or middle mouse button. Middle mouse rotation is very dizzying, so I recommend just sticking to left mouse.
Translate the camera holding CTRL + left click or middle mouse button.
Scale the heightmap by holding SHIFT + left click or middle mouse button. Note that you can scale such that the image is flipped over the relevant axis (negative scaling). I thought it looked cool, so I allow inverting image.
Change color scheme of heightmap by using TAB keyboard button. There are 7 schemes.

* DO NOT USE THIS PART *
Additionally, you can press "r" key to start recording. This will begin saving images to folder, which can take up a lot of memory and may lag your computer. I put this in to make it easier to make video. I do not recommend testing this feature, but just letting you know it is there.


--- EXTRA CREDIT ----------
For extra credit, I implemented a color scheme cycling feature for the heightmap. By hitting TAB, user can cycle through different color schemes for the heightmap. There are 7 color schemes, including the default black/white. The colors are consistent with scaling on the heightmap.
Additionally, I noticed that there was a black cube outline of the skybox. I got rid of this by slightly scaling the skybox faces so that they (very slightly) overlap. The outcome is that the outline is reduced significantly without damaging realistic-ness of skybox.