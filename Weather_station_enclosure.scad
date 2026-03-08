$fn = 96;

// Debug switches
SHOW_2D_ONLY  = true;   // set false once you see silhouette
SHOW_OUTER_3D = false;  // set true to see an extruded slab

tilt_deg   = 10;
fillet_r   = 30;

base_depth = 95;
base_h     = 26;

blade_h    = 135;
blade_back = 10;

module outer_2d() {
  hull() {
    // Base rectangle (electronics volume)
    square([base_depth, base_h], center=false);

    // Blade rectangle (screen “sheet”), tilted
    translate([base_depth - blade_back - 2, base_h])
      rotate(-tilt_deg)
        square([blade_back, blade_h], center=false);

    // Circle to encourage a smooth bend
    translate([base_depth - fillet_r - 6, base_h - fillet_r])
      circle(r=fillet_r);
  }
}

if (SHOW_2D_ONLY) {
  outer_2d();
} else if (SHOW_OUTER_3D) {
  linear_extrude(height=20) outer_2d();
}