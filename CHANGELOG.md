# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
## [2.1.0] - 2023-07-??

### Added
- Tag Addon
- Misc Addon
  - Consequtive wallruns limit removed 
  - Autoroll
  - No health regeneration
  - Permanent reaction time
  - Permanent game speed
  - One hit knock down
- Double jump 
- Sidestepbeamer 
- Reset button next to bindable keys
- Refill reaction time energy button
- 4 more selectable models
  - Ropeburn
  - Riot Cop
  - Swat
  - Swat Sniper

### Fixed
- Creating a save and reloading level and teleporting would cause Faith to autoroll
- Dying and teleporting showed a black screen
- Arms not being reset to default postion when teleporting
- Sound not being reset when teleporting during reaction time
- Reaction time energy not being saved using trainer
- Teleporting while sliding didn't remove the sliding sound
  
### Changed
- Styling in tabs to add more space between stuff  
- Improved reaction time when using the trainer
- Flying speed to reset after being toggled
- Default flying speed to be 2.0f instead of 3.0f

### Removed
- Option to toggle the overlay in trainer
