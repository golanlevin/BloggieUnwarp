<!-- Settings File & Instruction Manual in one!                -->
<!-- For BloggieUnwarper                                       -->
<!-- Golan Levin & Solomon Bisker                              -->
<!-- Made in openFrameworks, 2010                              -->
<!--                                                           -->
<!--                                                           -->
<!-- Key commands for using the App.                           -->
<!-- // INSTRUCTIONS for using the App!!                       -->
<!-- // Press 's' to save the geometry settings.               -->
<!-- // Press 'r' to reload the previously saved settings.     -->
<!-- // Use the +/- keys to change the export codec.           -->
<!-- // Press 'v' to export the unwarped video.                -->
<!-- // Use the arrow keys to nudge the center point.          -->
<!-- // Drag the unwarped video left or right to shift it.     -->
<!--                                                           -->
<!-- Documentation of UnwarperSettings.xml:                    -->
<!-- INPUT_FILENAME:   Name of your input (warped) movie       -->
<!-- MAXR_FACTOR:      Large (outer) radius, as % of height    -->
<!-- MINR_FACTOR:      Small (inner) radius, as % of height    -->
<!-- ROTATION_DEGREES: Overall rotation/shift offset           -->
<!-- CENTERX:          Location of X center                    -->
<!-- CENTERY:          Location of Y center                    -->
<!-- OUTPUT_W:         Width of output movie                   -->
<!-- OUTPUT_H:         Height of output movie                  -->
<!-- Note: The Bloggie has a vertical FOV of 55 degrees,       -->
<!-- thus an optimal ratio of W/H = 360/55 = 6.55:1            -->
<!-- INTERP_METHOD:    Interpolation method, 0,1,2             -->
<!-- 0 = CV_INTER_NN; 1 = CV_INTER_LINEAR; 2 = CV_INTER_CUBIC. -->
<!-- CODEC_QUALITY:    Compression quality, 0(min)-5(lossless) -->
<!-- R_WARP_A,B,C:     Bloggie-specific optics parameters.     -->
<!-- Don't touch those!                                        -->
<!--                                                           -->
<!-- Example settings:                                         -->
<INPUT_FILENAME>bike_trip.mov</INPUT_FILENAME>
<MAXR_FACTOR>0.947</MAXR_FACTOR>
<MINR_FACTOR>0.245</MINR_FACTOR>
<ROTATION_DEGREES>105.750000</ROTATION_DEGREES>
<CENTERX>325.250000</CENTERX>
<CENTERY>182.750000</CENTERY>
<OUTPUT_W>1280</OUTPUT_W>
<OUTPUT_H>196</OUTPUT_H>
<INTERP_METHOD>1</INTERP_METHOD>
<CODEC_QUALITY>4</CODEC_QUALITY>
<R_WARP_A>0.1850</R_WARP_A>
<R_WARP_B>0.8184</R_WARP_B>
<R_WARP_C>-0.0028</R_WARP_C>
