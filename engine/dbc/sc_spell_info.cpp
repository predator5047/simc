// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "sc_spell_info.hpp"

namespace {
struct proc_map_t
{
  int         flag;
  const char* proc;
};

static std::vector<std::string> _spell_type_map {
  "None", "Magic", "Melee", "Ranged"
};

std::vector<std::string> _school_map = {
  "Physical",
  "Holy",
  "Fire",
  "Nature",
  "Frost",
  "Shadow",
  "Arcane",
};

std::vector<std::string> _hotfix_effect_map = {
  "Id",
  "", // Hotfix field
  "Spell Id",
  "Index",
  "Type",
  "Sub Type",
  "Coefficient",
  "Delta",
  "Bonus",
  "SP Coefficient",
  "AP Coefficient",
  "Period",
  "Min Radius",
  "Max Radius",
  "Base Value",
  "Misc Value",
  "Misc Value 2",
  "Affects Spells",
  "Trigger Spell",
  "Chain Multiplier",
  "Points per Combo Points",
  "Points per Level",
  "Mechanic",
  "Chain Targets",
  "Target 1",
  "Target 2",
  "Value Multiplier",
  "PvP Coefficient"
};

std::vector<std::string> _hotfix_spell_map = {
  "Name",
  "Id",
  "", // Hotfix field
  "Velocity",
  "School",
  "Class",
  "Race",
  "Scaling Spell",
  "Max Scaling Level",
  "Spell Level",
  "Max Spell Level",
  "Min Range",
  "Max Range",
  "Cooldown",
  "GCD",
  "Category Cooldown",
  "Charges",
  "Charge Cooldown",
  "Category",
  "Duration",
  "Max stacks",
  "Proc Chance",
  "Proc Stacks",
  "Proc Flags",
  "Internal Cooldown",
  "RPPM",
  "", "", "", // Equipped items stuff, don't think we use these anywhere
  "Min Cast Time",
  "Max Cast Time",
  "", "", "", // Old cast-time related fields, now zero (cast_div, cast_scaling, cast_scaling_type)
  "", // Replace spell id, we're not flagging these as it's derived information
  "Attributes",
  "Affecting Spells",
  "Spell Family",
  "Stance Mask",
  "Mechanic",
  "Azerite Power Id",
  "Azerite Essence Id",
  "Description",
  "Tooltip",
  "Variables",
  "Rank",
  "Required Max Level",
  "Spell Type"
};

std::vector<std::string> _hotfix_power_map = {
  "Id",
  "Spell Id",
  "Aura Id",
  "", // Hotfix flags
  "Power Type",
  "Cost",
  "Max Cost",
  "Cost per Tick",
  "Percent Cost",
  "Max Percent Cost",
  "Percent Cost per Tick"
};

template <typename T>
bool map_has_string( const std::unordered_map<T, const std::string>& map, T key )
{
  return map.find( key ) != map.end();
}

template <typename T>
std::string map_string( const std::unordered_map<T, const std::string>& map, T key )
{
  auto it = map.find( key );

  if ( it != map.end() )
  {
    return it -> second + " (" + util::to_string( key ) + ")";
  }

  return "Unknown(" + util::to_string( key ) + ")";
}

template <typename DATA_TYPE, typename MAP_TYPE>
std::ostringstream& hotfix_map_str( const DATA_TYPE* data, std::ostringstream& s, const std::vector<std::string>& map )
{
  if ( data -> _hotfix == 0 )
  {
    return s;
  }

  for ( size_t i = 0; i < map.size(); ++i )
  {
    MAP_TYPE shift = (static_cast<MAP_TYPE>( 1 ) << i);

    if ( ! ( data -> _hotfix & shift ) )
    {
      continue;
    }

    if ( s.tellp() > 0 )
    {
      s << ", ";
    }

    if ( map[ i ].empty() )
    {
      s << "Unknown(" << i << ")";
    }
    else
    {
      s << map[ i ];
      auto hotfix_entry = hotfix::hotfix_entry( data, static_cast<unsigned int>( i ) );
      if ( hotfix_entry != nullptr )
      {
        switch ( hotfix_entry -> field_type )
        {
          case hotfix::UINT:
            s << " (" << hotfix_entry -> orig_value.u << " -> " << hotfix_entry -> hotfixed_value.u << ")";
            break;
          case hotfix::INT:
            s << " (" << hotfix_entry -> orig_value.i << " -> " << hotfix_entry -> hotfixed_value.i << ")";
            break;
          case hotfix::FLOAT:
            s << " (" << hotfix_entry -> orig_value.f << " -> " << hotfix_entry -> hotfixed_value.f << ")";
            break;
          // Don't print out the changed string for now, seems pointless
          case hotfix::STRING:
            break;
        }
      }
    }
  }

  return s;
}

template <typename T>
std::string concatenate( const std::vector<const T*>& data,
                         const std::function<void(std::stringstream&, const T*)> fn,
                         const std::string& delim = ", " )
{
  if ( data.size() == 0 )
  {
    return "";
  }

  std::stringstream s;

  for ( size_t i = 0, end = data.size(); i < end; ++i )
  {
    fn( s, data[ i ] );

    if ( i < end - 1 )
    {
      s << delim;
    }
  }

  return s.str();
}

std::string spell_hotfix_map_str( const spell_data_t* spell )
{
  std::ostringstream s;

  if ( spell -> _hotfix == dbc::HOTFIX_SPELL_NEW )
  {
    s << "NEW SPELL";
  }
  else
  {
    hotfix_map_str<spell_data_t, uint64_t>( spell, s, _hotfix_spell_map );
  }

  return s.str();
}

std::string effect_hotfix_map_str( const spelleffect_data_t* effect )
{
  std::ostringstream s;

  if ( effect -> _hotfix == dbc::HOTFIX_EFFECT_NEW )
  {
    s << "NEW EFFECT";
  }
  else
  {
    hotfix_map_str<spelleffect_data_t, unsigned>( effect, s, _hotfix_effect_map );
  }

  return s.str();
}

std::string power_hotfix_map_str( const spellpower_data_t* power )
{
  std::ostringstream s;

  if ( power -> _hotfix == dbc::HOTFIX_POWER_NEW )
  {
    s << "NEW POWER";
  }
  else
  {
    hotfix_map_str<spellpower_data_t, unsigned>( power, s, _hotfix_power_map );
  }

  return s.str();
}

const struct proc_map_t _proc_flag_map[] =
{
  { PF_KILLED,               "Killed"                      },
  { PF_KILLING_BLOW,         "Killing Blow"                },
  { PF_MELEE,                "White Melee"                 },
  { PF_MELEE_TAKEN,          "White Melee Taken"           },
  { PF_MELEE_ABILITY,        "Yellow Melee"                },
  { PF_MELEE_ABILITY_TAKEN,  "Yellow Melee Taken"          },
  { PF_RANGED,               "White Ranged"                },
  { PF_RANGED_TAKEN,         "White Ranged Taken "         },
  { PF_RANGED_ABILITY,       "Yellow Ranged"               },
  { PF_RANGED_ABILITY_TAKEN, "Yellow Ranged Taken"         },
  { PF_NONE_HEAL,            "Generic Heal"                },
  { PF_NONE_HEAL_TAKEN,      "Generic Heal Taken"          },
  { PF_NONE_SPELL,           "Generic Hostile Spell"       },
  { PF_NONE_SPELL_TAKEN,     "Generic Hostile Spell Taken" },
  { PF_MAGIC_HEAL,           "Magic Heal"                  },
  { PF_MAGIC_HEAL_TAKEN,     "Magic Heal Taken"            },
  { PF_MAGIC_SPELL,          "Magic Hostile Spell"         },
  { PF_MAGIC_SPELL_TAKEN,    "Magic Hostile Spell Taken"   },
  { PF_PERIODIC,             "Periodic"                    },
  { PF_PERIODIC_TAKEN,       "Periodic Taken"              },
  { PF_ANY_DAMAGE_TAKEN,     "Any Damage Taken"            },
  { PF_TRAP_TRIGGERED,       "Trap Triggered"              },
  { PF_JUMP,                 "Proc on jump"                },
  { PF_MAINHAND,             "Melee Main-Hand"             },
  { PF_OFFHAND,              "Melee Off-Hand"              },
  { PF_DEATH,                "Death"                       },
  { 0,                       nullptr                       }
};

const struct { const char* name; player_e pt; } _class_map[] =
{
  { nullptr, PLAYER_NONE },
  { "Warrior", WARRIOR },
  { "Paladin", PALADIN },
  { "Hunter", HUNTER },
  { "Rogue", ROGUE },
  { "Priest", PRIEST },
  { "Death Knight", DEATH_KNIGHT },
  { "Shaman", SHAMAN },
  { "Mage", MAGE },
  { "Warlock", WARLOCK },
  { "Monk", MONK },
  { "Druid", DRUID },
  { "Demon Hunter", DEMON_HUNTER },
  { nullptr, PLAYER_NONE },
};

const std::unordered_map<unsigned, std::string> _race_map {
  {  0, "Human"     },
  {  1, "Orc"       },
  {  2, "Dwarf"     },
  {  3, "Night Elf" },
  {  4, "Undead"    },
  {  5, "Tauren"    },
  {  6, "Gnome"     },
  {  7, "Troll"     },
  {  8, "Goblin"    },
  {  9, "Blood Elf" },
  { 10, "Draenei"   },
  { 11, "Dark Iron Dwarf" },
  { 13, "Mag'har Orc" },
  { 21, "Worgen"    },
  { 24, "Pandaren"  },
  { 26, "Nightborne" },
  { 27, "Highmountain Tauren" },
  { 28, "Void Elf"            },
  { 29, "Lightforged Draenei" },
  { 30, "Zandalari Troll" },
  { 31, "Kul Tiran" }
};

static const std::unordered_map<unsigned, const std::string> _targeting_strings = {
  {  1,  "Self"                },
  {  5,  "Active Pet"          },
  {  6,  "Enemy"               },
  {  7,  "AOE enemy (instant)" },
  {  8,  "AOE Custom"          },
  { 15,  "AOE enemy"           },
  { 18,  "Custom"              },
  { 16,  "AOE enemy (instant)" },
  { 21,  "Friend"              },
  { 22,  "At Caster"           },
  { 28,  "AOE enemy"           },
  { 30,  "AOE friendly"        },
  { 31,  "AOE friendly"        },
  { 42,  "Water Totem"         },
  { 45,  "Chain Friendly"      },
  { 104, "Cone Front"          },
  { 53,  "At Enemy"            },
  { 56,  "Raid Members"        },
};

static const std::unordered_map<int, const std::string> _resource_strings =
{
  { -2, "Health",        },
  {  0, "Base Mana",     },
  {  1, "Rage",          },
  {  2, "Focus",         },
  {  3, "Energy",        },
  {  4, "Combo Points",  },
  {  5, "Rune",          },
  {  6, "Runic Power",   },
  {  7, "Soul Shard",    },
  {  8, "Eclipse",       },
  {  9, "Holy Power",    },
  { 11, "Maelstrom",     },
  { 12, "Chi",           },
  { 13, "Insanity",      },
  { 14, "Burning Ember", },
  { 15, "Demonic Fury",  },
  { 17, "Fury",          },
  { 18, "Pain",          },
};

const std::map<unsigned, std::string> _attribute_strings = {
  {   1, "Ranged Ability"                    },
  {   5, "Tradeskill ability"                },
  {   6, "Passive"                           },
  {   7, "Hidden"                            },
  {  17, "Requires stealth"                  },
  {  20, "Stop attacks"                      },
  {  21, "Cannot dodge/parry/block"          },
  {  28, "Cannot be used in combat"          },
  {  31, "Cannot cancel aura"                },
  {  34, "Channeled"                         },
  {  37, "Does not break stealth"            },
  {  38, "Channeled"                         },
  {  93, "Cannot crit"                       },
  {  95, "Food buff"                         },
  { 105, "Not a proc"                        },
  { 106, "Requires main-hand weapon"         },
  { 112, "Disable player procs"              },
  { 113, "Disable target procs"              },
  { 114, "Always hits"                       },
  { 120, "Requires off-hand weapon"          },
  { 121, "Treat as periodic"                 },
  { 151, "Disable weapon procs"              },
  { 169, "Tick on application"               },
  { 173, "Periodic effect affected by haste" },
  { 186, "Requires line of sight"            },
  { 221, "Disable player multipliers"        },
  { 265, "Periodic effect can crit"          },
  { 354, "Scales with item level"            }
};

static const std::unordered_map<int, const std::string> _property_type_strings =
{
  {  0, "Spell Direct Amount"       },
  {  1, "Spell Duration"            },
  {  2, "Spell Generated Threat"    },
  {  3, "Spell Effect 1"            },
  {  4, "Spell Initial Stacks"      },
  {  5, "Spell Range"               },
  {  6, "Spell Radius"              },
  {  7, "Spell Critical Chance"     },
  {  8, "Spell Effects"             },
  {  9, "Spell Pushback"            },
  { 10, "Spell Cast Time"           },
  { 11, "Spell Cooldown"            },
  { 12, "Spell Effect 2"            },
  { 14, "Spell Resource Cost"       },
  { 15, "Spell Critical Damage"     },
  { 16, "Spell Penetration"         },
  { 17, "Spell Targets"             },
  { 18, "Spell Proc Chance"         },
  { 19, "Spell Tick Time"           },
  { 20, "Spell Target Bonus"        },
  { 21, "Spell Global Cooldown"     },
  { 22, "Spell Periodic Amount"     },
  { 23, "Spell Effect 3"            },
  { 24, "Spell Power"               },
  { 26, "Spell Proc Frequency"      },
  { 27, "Spell Damage Taken"        },
  { 28, "Spell Dispel Chance"       },
  { 32, "Spell Effect 4"            },
  { 33, "Spell Effect 5"            },
  { 34, "Spell Resource Generation" },
  { 37, "Spell Max Stacks"          },
};

static const std::unordered_map<unsigned, const std::string> _effect_type_strings =
{
  {   0, "None"                     },
  {   1, "Instant Kill"             },
  {   2, "School Damage"            },
  {   3, "Dummy"                    },
  {   4, "Portal Teleport"          },
  {   5, "Teleport Units"           },
  {   6, "Apply Aura"               },
  {   7, "Environmental Damage"     },
  {   8, "Power Drain"              },
  {   9, "Health Leech"             },
  {  10, "Direct Heal"              },
  {  11, "Bind"                     },
  {  12, "Portal"                   },
  {  13, "Ritual Base"              },
  {  14, "Ritual Specialize"        },
  {  15, "Ritual Activate"          },
  {  16, "Quest Complete"           },
  {  17, "Weapon Damage"            },
  {  18, "Resurrect"                },
  {  19, "Extra Attacks"            },
  {  20, "Dodge"                    },
  {  21, "Evade"                    },
  {  22, "Parry"                    },
  {  23, "Block"                    },
  {  24, "Create Item"              },
  {  25, "Weapon Type"              },
  {  26, "Defense"                  },
  {  27, "Persistent Area Aura"     },
  {  28, "Summon"                   },
  {  29, "Leap"                     },
  {  30, "Energize Power"           },
  {  31, "Weapon Damage%"           },
  {  32, "Trigger Missiles"         },
  {  33, "Open Lock"                },
  {  34, "Summon Item"              },
  {  35, "Apply Party Aura"         },
  {  36, "Learn Spell"              },
  {  37, "Spell Defense"            },
  {  38, "Dispel"                   },
  {  39, "Language"                 },
  {  40, "Dual Wield"               },
  {  48, "Stealth"                  },
  {  49, "Detect"                   },
  {  52, "Guaranteed Hit"           },
  {  53, "Enchant Item"             },
  {  56, "Summon Pet"               },
  {  58, "Weapon Damage"            },
  {  62, "Power Burn"               },
  {  63, "Threat"                   },
  {  64, "Trigger Spell"            },
  {  65, "Apply Raid Aura"          },
  {  68, "Interrupt Cast"           },
  {  69, "Distract"                 },
  {  70, "Pull"                     },
  {  71, "Pick Pocket"              },
  {  77, "Server Side Script"       },
  {  78, "Attack"                   },
  {  80, "Add Combo Points"         },
  {  85, "Summon Player"            },
  {  91, "Threat All"               },
  {  94, "Self Resurrect"           },
  {  96, "Charge"                   },
  {  97, "Summon All Totems"        },
  {  98, "Knock Back"               },
  { 101, "Feed Pet"                 },
  { 102, "Dismiss Pet"              },
  { 109, "Summon Dead Pet"          },
  { 110, "Destroy All Totems"       },
  { 113, "Resurrect"                },
  { 119, "Apply Pet Area Aura"      },
  { 121, "Normalized Weapon Damage" },
  { 124, "Pull Player"              },
  { 125, "Modify Threat"            },
  { 126, "Steal Beneficial Aura"    },
  { 128, "Apply Friendly Area Aura" },
  { 129, "Apply Enemy Area Aura"    },
  { 130, "Redirect Threat"          },
  { 135, "Call Pet"                 },
  { 136, "Direct Heal%"             },
  { 137, "Energize Power%"          },
  { 138, "Leap Back"                },
  { 142, "Trigger Spell w/ Value"   },
  { 143, "Apply Owner Area Aura"    },
  { 146, "Activate Rune"            },
  { 151, "Trigger Spell"            },
  { 155, "Titan Grip"               },
  { 156, "Add Socket"               },
  { 157, "Create Item"              },
  { 202, "Apply Player/Pet Aura"    },
};

static const std::unordered_map<unsigned, const std::string> _effect_subtype_strings =
{
  {   0, "None"                                         },
  {   2, "Possess"                                      },
  {   3, "Periodic Damage"                              },
  {   4, "Dummy"                                        },
  {   5, "Confuse"                                      },
  {   6, "Charm"                                        },
  {   7, "Fear"                                         },
  {   8, "Periodic Heal"                                },
  {   9, "Attack Speed"                                 },
  {  10, "Threat"                                       },
  {  11, "Taunt"                                        },
  {  12, "Stun"                                         },
  {  13, "Damage Done"                                  },
  {  14, "Damage Taken"                                 },
  {  15, "Damage Shield"                                },
  {  16, "Stealth"                                      },
  {  17, "Stealth Detection"                            },
  {  18, "Invisibility"                                 },
  {  19, "Invisibility Detection"                       },
  {  20, "Periodic Heal%"                               },
  {  21, "Periodic Power% Regen"                        },
  {  22, "Resistance"                                   },
  {  23, "Periodic Trigger Spell"                       },
  {  24, "Periodic Energize Power"                      },
  {  25, "Pacify"                                       },
  {  26, "Root"                                         },
  {  27, "Silence"                                      },
  {  28, "Spell Reflection"                             },
  {  29, "Attribute"                                    },
  {  30, "Skill"                                        },
  {  31, "Increase Speed%"                              },
  {  32, "Increase Mounted Speed%"                      },
  {  33, "Decrease Movement Speed%"                     },
  {  34, "Increase Health"                              },
  {  35, "Increase Energy"                              },
  {  36, "Shapeshift"                                   },
  {  37, "Immunity Against External Movement"           },
  {  39, "School Immunity"                              },
  {  40, "Damage Immunity"                              },
  {  41, "Disable Stealth"                              },
  {  42, "Proc Trigger Spell"                           },
  {  43, "Proc Trigger Damage"                          },
  {  44, "Track Creatures"                              },
  {  47, "Modify Parry%"                                },
  {  49, "Modify Dodge%"                                },
  {  50, "Modify Critical Heal Bonus"                   },
  {  51, "Modify Block%"                                },
  {  52, "Modify Crit%"                                 },
  {  53, "Periodic Health Leech"                        },
  {  54, "Modify Hit%"                                  },
  {  55, "Modify Spell Hit%"                            },
  {  56, "Change Model"                                 },
  {  57, "Modify Spell Crit%"                           },
  {  60, "Pacify Silence"                               },
  {  61, "Scale% (Stacking)"                            },
  {  63, "Modify Max Cost"                              },
  {  64, "Periodic Mana Leech"                          },
  {  65, "Modify Spell Speed%"                          },
  {  66, "Feign Death"                                  },
  {  67, "Disarm"                                       },
  {  68, "Stalked"                                      },
  {  69, "Absorb Damage"                                },
  {  72, "Modify Power Cost%"                           },
  {  73, "Modify Power Cost"                            },
  {  74, "Reflect Spells"                               },
  {  77, "Mechanic Immunity"                            },
  {  79, "Modify Damage Done%"                          },
  {  80, "Modify Attribute%"                            },
  {  81, "Transfer Damage%"                             },
  {  84, "Restore Health"                               },
  {  85, "Restore Power"                                },
  {  87, "Modify Damage Taken%"                         },
  {  88, "Modify Health Regeneration%"                  },
  {  89, "Periodic Max Health% Damage"                  },
  {  99, "Modify Attack Power"                          },
  { 101, "Modify Armor%"                                },
  { 102, "Modify Melee Attack Power vs Race"            },
  { 103, "Temporary Thread Reduction"                   },
  { 104, "Modify Attack Power"                          },
  { 106, "Levitate"                                     },
  { 107, "Add Flat Modifier"                            },
  { 108, "Add Percent Modifier"                         },
  { 110, "Modify Power Regen"                           },
  { 115, "Modify Healing Received"                      },
  { 116, "Combat Health Regen%"                         },
  { 117, "Mechanic Resistance"                          },
  { 118, "Modify Healing Recevied%"                     },
  { 123, "Modify Target Resistance"                     },
  { 124, "Modify Ranged Attack Power"                   },
  { 129, "Increase Movement Speed% (Stacking)"          },
  { 130, "Increase Mount Speed% (Stacking)"             },
  { 131, "Modify Ranged Attack Power vs Race"           },
  { 132, "Modify Max Resource%"                         },
  { 133, "Modify Max Health%"                           },
  { 135, "Modify Healing Power"                         },
  { 136, "Modify Healing% Done"                         },
  { 137, "Modify Total Stat%"                           },
  { 138, "Modify Melee Haste%"                          },
  { 140, "Modify Ranged Haste%"                         },
  { 142, "Modify Base Resistance"                       },
  { 143, "Modify Cooldown Recharge Rate"                },
  { 144, "Reduce Fall Damage"                           },
  { 149, "Modify Casting Pushback"                      },
  { 150, "Modify Block Effectiveness"                   },
  { 152, "Modify Aggro Distance"                        },
  { 157, "Modify Absorb% Done"                          },
  { 163, "Modify Crit Damage Done%"                     },
  { 166, "Modify Melee Attack Power%"                   },
  { 167, "Modify Ranged Attack Power%"                  },
  { 168, "Modify Damage Done% vs Race"                  },
  { 172, "Increase Mounted Speed%"                      },
  { 177, "Charmed"                                      },
  { 178, "Modify Max Mana%"                             },
  { 180, "Modify Spell Damage vs Race"                  },
  { 184, "Modify Attacker Melee Hit Chance"             },
  { 185, "Modify Attacker Ranged Hit Chance"            },
  { 186, "Modify Attacker Spell Hit Chance"             },
  { 187, "Modify Attacker Melee Crit Chance"            },
  { 189, "Modify Rating"                                },
  { 192, "Modify Ranged and Melee Haste%"               },
  { 193, "Modify All Haste%"                            },
  { 197, "Modify Attacker Crit Chance"                  },
  { 200, "Modify Experience Gained from Kills"          },
  { 216, "Modify Casting Speed"                         },
  { 218, "Apply Percent Modifier w/ Label"              },
  { 224, "Grant Talent"                                 },
  { 226, "Periodic Dummy"                               },
  { 228, "Stealth Detection"                            },
  { 229, "Modify AoE Damage Taken%"                     },
  { 231, "Trigger Spell with Value"                     },
  { 232, "Modify Mechanic Duration% (Stacking)"         },
  { 234, "Modify Mechanic Duration%"                    },
  { 239, "Scale%"                                       },
  { 240, "Modify Expertise%"                            },
  { 241, "Forced Movement"                              },
  { 244, "Comprehend Language"                          },
  { 245, "Modify Debuff Duration%"                      },
  { 247, "Copy Appearance"                              },
  { 250, "Increase Max Health (Stacking)"               },
  { 253, "Modify Critical Block Chance"                 },
  { 259, "Modify Periodic Healing Recevied%"            },
  { 263, "Disable Abilities"                            },
  { 268, "Modify Armor by Primary Stat%"                },
  { 269, "Modify Damage Done% to Caster"                },
  { 270, "Modify Damage Taken% from Caster"             },
  { 271, "Modify Damage Taken% from Caster's Spells"    },
  { 283, "Modify Healing Taken% from Caster's Spells"   },
  { 290, "Modify Critical Strike%"                      },
  { 291, "Modify Experience Gained from Quests"         },
  { 301, "Absorb Healing"                               },
  { 305, "Modify Min Speed%"                            },
  { 308, "Modify Crit Chance% from Caster's Spells"     },
  { 318, "Modify Mastery%"                              },
  { 319, "Modify Melee Attack Speed%"                   },
  { 320, "Modify Ranged Attack Speed%"                  },
  { 329, "Modify Resource Generation%"                  },
  { 330, "Cast while Moving (Whitelist)"                },
  { 334, "Modify Auto Attack Critical Chance"           },
  { 342, "Modify Ranged and Melee Attack Speed%"        },
  { 343, "Modify Auto Attack Damage Taken% from Caster" },
  { 344, "Modify Auto Attack Damage Done%"              },
  { 345, "Ignore Armor%"                                },
  { 354, "Modify Healing% Based on Target Health%"      },
  { 360, "Duplicate Ability"                            },
  { 361, "Override Auto-Attack with Spell"              },
  { 366, "Override Spell Power per Attack Power%"       },
  { 374, "Reduce Fall Damage%"                          },
  { 377, "Cast while Moving"                            },
  { 379, "Modify Mana Regen%"                           },
  { 383, "Ignore Spell Cooldown"                        },
  { 404, "Override Attack Power per Spell Power%"       },
  { 405, "Modify Combat Rating Multiplier"              },
  { 409, "Slow Fall"                                    },
  { 411, "Modify Cooldown Charge (Category)"            },
  { 416, "Hasted Cooldown Duration"                     },
  { 417, "Hasted Global Cooldown"                       },
  { 418, "Modify Max Resource"                          },
  { 419, "Modify Mana Pool%"                            },
  { 421, "Modify Absorb% Done"                          },
  { 422, "Modify Absorb% Done"                          },
  { 429, "Modify Pet Damage Done%"                      },
  { 441, "Modify Multistrike%"                          },
  { 443, "Modify Leech%"                                },
  { 453, "Modify Recharge Time (Category)"              },
  { 454, "Modify Recharge Time% (Category)"             },
  { 455, "Root"                                         },
  { 457, "Hasted Cooldown Duration (Category)"          },
  { 465, "Increase Armor"                               },
  { 468, "Trigger Spell Based on Health%"               },
  { 471, "Modify Versatility%"                          },
  { 485, "Resist Forced Movement%"                      },
};


static const std::unordered_map<unsigned, const char*> _mechanic_strings { {
  { 130, "Charm"          },
  { 134, "Disorient"      },
  { 142, "Disarm"         },
  { 147, "Distract"       },
  { 154, "Flee"           },
  { 158, "Grip"           },
  { 162, "Root"           },
  { 165, "Slow"           },
  { 168, "Silence"        },
  { 173, "Sleep"          },
  { 176, "Snare"          },
  { 179, "Stun"           },
  { 183, "Freeze"         },
  { 186, "Incapacitate"   },
  { 196, "Bleed"          },
  { 201, "Heal"           },
  { 205, "Polymorph"      },
  { 213, "Banish"         },
  { 218, "Shield"         },
  { 223, "Shackle"        },
  { 228, "Mount"          },
  { 232, "Infect"         },
  { 237, "Turn"           },
  { 240, "Horrify"        },
  { 246, "Invulneraility" },
  { 255, "Interrupt"      },
  { 263, "Daze"           },
  { 265, "Discover"       },
  { 271, "Sap"            },
  { 274, "Enrage"         },
  { 278, "Wound"          },
  { 282, "Taunt"          }
} };

std::string mechanic_str( unsigned mechanic ) {
  auto it = _mechanic_strings.find( mechanic );
  if ( it != _mechanic_strings.end() )
  {
    return it -> second;
  }

  return "Unknown(" + util::to_string( mechanic ) + ")";
}

std::string spell_flags( const spell_data_t* spell )
{
  std::ostringstream s;

  s << "[";

  if ( spell -> scaling_class() != 0 )
    s << "Scaling Spell (" << spell -> scaling_class() << "), ";

  if ( spell -> class_family() != 0 )
    s << "Spell Family (" << spell -> class_family() << "), ";

  if ( spell -> flags( spell_attribute::SX_PASSIVE ) )
    s << "Passive, ";

  if ( spell -> flags( spell_attribute::SX_HIDDEN ) )
    s << "Hidden, ";

  if ( s.tellp() > 1 )
  {
    s.seekp( -2, std::ios_base::cur );
    s << "]";
  }
  else
    return std::string();

  return s.str();
}

void spell_flags_xml( const spell_data_t* spell, xml_node_t* parent )
{
  if ( spell -> scaling_class() != 0 )
    parent -> add_parm( "scaling_spell", spell -> scaling_class() );

  if ( spell -> flags( spell_attribute::SX_PASSIVE ) )
    parent -> add_parm( "passive", "true" );
}

std::string azerite_essence_str( const spell_data_t* spell,
                             const arv::array_view<azerite_essence_power_entry_t>& data )
{
  // Locate spell in the array
  auto it = range::find_if( data, [ spell ]( const azerite_essence_power_entry_t& e ) {
    return e.spell_id_base[ 0 ] == spell->id() || e.spell_id_base[ 1 ] == spell->id() ||
           e.spell_id_upgrade[ 0 ] == spell->id() || e.spell_id_upgrade[ 1 ] == spell->id();
  } );

  if ( it == data.end() )
  {
    return "";
  }

  std::ostringstream s;

  s << "(";

  s << "Type: ";

  if ( it->spell_id_base[ 0 ] == spell->id() )
  {
    s << "Major/Base";
  }
  else if ( it->spell_id_base[ 1 ] == spell->id() )
  {
    s << "Minor/Base";
  }
  else if ( it->spell_id_upgrade[ 0 ] == spell->id() )
  {
    s << "Major/Upgrade";
  }
  else if ( it->spell_id_upgrade[ 1 ] == spell->id() )
  {
    s << "Minor/Upgrade";
  }
  else
  {
    s << "Unknown";
  }
  s << ", ";


  s << "Rank: " << it->rank;


  s << ")";

  return s.str();
}

} // unnamed namespace

std::ostringstream& spell_info::effect_to_str( const dbc_t& dbc,
                                               const spell_data_t*       spell,
                                               const spelleffect_data_t* e,
                                               std::ostringstream&       s,
                                               int level )
{
  std::streamsize ssize = s.precision( 7 );
  char tmp_buffer[512],
       tmp_buffer2[64];

  snprintf( tmp_buffer2, sizeof( tmp_buffer2 ), "(id=%u)", e -> id() );
  snprintf( tmp_buffer, sizeof( tmp_buffer ), "#%d %-*s: ", (int16_t)e -> index() + 1, 14, tmp_buffer2 );
  s << tmp_buffer;

  s << map_string( _effect_type_strings, e -> raw_type() );
  // Put some nice handling on some effect types
  switch ( e -> type() )
  {
    case E_SCHOOL_DAMAGE:
      s << ": " << util::school_type_string( spell -> get_school_type() );
      break;
    case E_TRIGGER_SPELL:
    case E_TRIGGER_SPELL_WITH_VALUE:
      if ( e -> trigger_spell_id() )
      {
        if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
          s << ": " << dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
        else
          s << ": (" << e -> trigger_spell_id() << ")";
      }
      break;
    default:
      break;
  }

  if ( e -> subtype() > 0 )
  {
    s << " | " << map_string( _effect_subtype_strings, e -> raw_subtype() );
    switch ( e -> subtype() )
    {
      case A_PERIODIC_DAMAGE:
        s << ": " << util::school_type_string( spell -> get_school_type() );
        if ( e -> period() != timespan_t::zero() )
          s << " every " << e -> period().total_seconds() << " seconds";
        break;
      case A_PERIODIC_HEAL:
      case A_PERIODIC_ENERGIZE:
      case A_PERIODIC_DUMMY:
      case A_OBS_MOD_HEALTH:
      case A_PERIODIC_LEECH:
        if ( e -> period() != timespan_t::zero() )
          s << ": every " << e -> period().total_seconds() << " seconds";
        break;
      case A_PROC_TRIGGER_SPELL:
        if ( e -> trigger_spell_id() )
        {
          if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
          {
            s << ": " << dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
          }
          else
          {
            s << ": (" << e -> trigger_spell_id() << ")";
          }
        }
        break;
      case A_PERIODIC_TRIGGER_SPELL:
        s << ": ";
        if ( e -> trigger_spell_id() && dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
        {
          s << dbc.spell( e -> trigger_spell_id() ) -> name_cstr();
        }
        else
        {
          s << "Unknown(" << e->trigger_spell_id() << ")";
        }

        if ( e -> period() != timespan_t::zero() )
          s << " every " << e -> period().total_seconds() << " seconds";
        break;
      case A_ADD_FLAT_MODIFIER:
      case A_ADD_PCT_MODIFIER:
      case A_ADD_PCT_LABEL_MODIFIER:
        s << ": " << map_string( _property_type_strings, e -> misc_value1() );
        break;
      default:
        break;
    }
  }

  s << std::endl;

  s << "                   Base Value: " << e -> base_value();
  s << " | Scaled Value: ";

  if ( level <= MAX_LEVEL )
  {
    double v_min = dbc.effect_min( e, level );
    double v_max = dbc.effect_max( e, level );

    s << v_min;
    if ( v_min != v_max )
      s << " - " << v_max;
  }
  else
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( level );
    double item_budget = ilevel_data.p_epic[ 0 ];
    auto coefficient = 1.0;

    if ( spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 )
    {
      // Technically this should check for the item type, but that's not possible right now
      coefficient = dbc.combat_rating_multiplier( level, CR_MULTIPLIER_TRINKET );
    }
    else if ( spell -> scaling_class() == PLAYER_SPECIAL_SCALE8 )
    {
      item_budget = ilevel_data.damage_replace_stat;
    }
    else if ( dbc.ptr && spell->scaling_class() == PLAYER_NONE &&
              spell->flags( spell_attribute::SX_SCALE_ILEVEL ) )
    {
      item_budget = ilevel_data.damage_secondary;
    }

    s << item_budget * e -> m_coefficient() * coefficient;

  }

  if ( e -> m_coefficient() != 0 || e -> m_delta() != 0 )
  {
    s << " (coefficient=" << e -> m_coefficient();
    if ( e -> m_delta() != 0 )
      s << ", delta coefficient=" << e -> m_delta();
    s << ")";
  }

  if ( level <= MAX_LEVEL )
  {
    if ( e -> m_unk() )
    {
      s << " | Bonus Value: " << dbc.effect_bonus( e -> id(), level );
      s << " (" << e -> m_unk() << ")";
    }
  }

  if ( e -> real_ppl() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%f", e -> real_ppl() );
    s << " | Points Per Level: " << e -> real_ppl();
  }

  if ( e -> m_value() != 0 )
  {
    s << " | Value Multiplier: " << e -> m_value();
  }

  if ( e -> sp_coeff() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%.5f", e -> sp_coeff() );
    s << " | SP Coefficient: " << tmp_buffer;
  }

  if ( e -> ap_coeff() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%.5f", e -> ap_coeff() );
    s << " | AP Coefficient: " << tmp_buffer;
  }

  if ( e -> pvp_coeff() != 0 )
  {
    snprintf( tmp_buffer, sizeof( tmp_buffer ), "%.5f", e -> pvp_coeff() );
    s << " | PvP Coefficient: " << tmp_buffer;
  }

  if ( e -> chain_target() != 0 )
    s << " | Chain Multiplier: " << e -> chain_multiplier();

  if ( e -> misc_value1() != 0 || e -> type() == E_ENERGIZE )
  {
    if ( e -> subtype() == A_MOD_DAMAGE_DONE ||
         e -> subtype() == A_MOD_DAMAGE_TAKEN ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_DONE ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_TAKEN )
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%#.x", e -> misc_value1() );
    else if ( e -> type() == E_ENERGIZE )
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%s", util::resource_type_string( util::translate_power_type( static_cast<power_e>( e -> misc_value1() ) ) ) );
    else
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%d", e -> misc_value1() );
    s << " | Misc Value: " << tmp_buffer;
  }

  if ( e -> misc_value2() != 0 )
  {
    if ( e -> subtype() == A_ADD_PCT_LABEL_MODIFIER )
    {
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%d (Label)", e -> misc_value2() );
    }
    else
    {
      snprintf( tmp_buffer, sizeof( tmp_buffer ), "%#.x", e -> misc_value2() );
    }
    s << " | Misc Value 2: " << tmp_buffer;
  }

  if ( e -> pp_combo_points() != 0 )
    s << " | Points Per Combo Point: " << e -> pp_combo_points();

  if ( e -> trigger_spell_id() != 0 )
    s << " | Trigger Spell: " << e -> trigger_spell_id();

  if ( e -> radius() > 0 || e -> radius_max() > 0 )
  {
    s << " | Radius: " << e -> radius();
    if ( e -> radius_max() > 0 && e -> radius_max() != e -> radius() )
      s << " - " << e -> radius_max();
    s << " yards";
  }

  if ( e -> mechanic() > 0 )
  {
    s << " | Mechanic: " << mechanic_str( e -> mechanic() );
  }

  if ( e -> chain_target() > 0 )
  {
    s << " | Chain Targets: " << e -> chain_target();
  }

  if ( e -> target_1() != 0 || e -> target_2() != 0 )
  {
    s << " | Target: ";
    if ( e -> target_1() && ! e -> target_2() )
    {
      s << map_string( _targeting_strings, e -> target_1() );
    }
    else if ( ! e -> target_1() && e -> target_2() )
    {
      s << "[" << map_string( _targeting_strings, e -> target_2() ) << "]";
    }
    else
    {
      s << map_string( _targeting_strings, e -> target_1() ) << " -> "
        << map_string( _targeting_strings, e -> target_2() );
    }
  }

  s << std::endl;

  if ( e -> type() == E_APPLY_AURA && e -> subtype() == A_MOD_DAMAGE_PERCENT_DONE &&
       e -> misc_value1() != 0 )
  {
    s << "                   Affected School(s): ";
    if ( e -> misc_value1() == 0x7f )
    {
      s << "All";
    }
    else
    {
      std::vector<std::string> schools;
      for ( size_t i = 0; i < _school_map.size(); ++i )
      {
        if ( e -> misc_value1() & ( 1 << i ) )
        {
          schools.push_back( _school_map[ i ] );
        }
      }

      for ( size_t i = 0; i < schools.size(); ++i )
      {
        s << schools[ i ];

        if ( i < schools.size() - 1 )
        {
          s << ", ";
        }
      }
    }

    s << std::endl;
  }


  std::vector< const spell_data_t* > affected_spells = dbc.effect_affects_spells( spell -> class_family(), e );
  if ( affected_spells.size() > 0 )
  {
    s << "                   Affected Spells: ";
    for ( size_t i = 0, end = affected_spells.size(); i < end; i++ )
    {
      s << affected_spells[ i ] -> name_cstr() << " (" << affected_spells[ i ] -> id() << ")";
      if ( i < end - 1 )
        s << ", ";
    }
    s << std::endl;
  }

  if ( e -> type() == E_APPLY_AURA && e -> subtype() == A_ADD_PCT_LABEL_MODIFIER )
  {
    auto affected_spells = dbc.spells_by_label( e -> misc_value2() );
    s << "                   Affected Spells (Label): ";
    for ( size_t i = 0, end = affected_spells.size(); i < end; i++ )
    {
      s << affected_spells[ i ] -> name_cstr() << " (" << affected_spells[ i ] -> id() << ")";
      if ( i < end - 1 )
        s << ", ";
    }
    s << std::endl;
  }

  if ( e -> type() == E_APPLY_AURA && e -> subtype() == A_HASTED_CATEGORY )
  {
    auto affected_spells = dbc.spells_by_category( e -> misc_value1() );
    s << "                   Affected Spells (Category): ";
    s << concatenate<spell_data_t>( affected_spells,
        []( std::stringstream& s, const spell_data_t* spell ) {
          s << spell -> name_cstr() << " (" << spell -> id() << ")";
        } );
    s << std::endl;
  }

  if ( spell->class_family() > 0 )
  {
    std::stringstream flags_s;

    for ( size_t i = 0; i < NUM_CLASS_FAMILY_FLAGS; ++i )
    {
      for ( size_t bit = 0; bit < 32; ++bit )
      {
        if ( ( 1 << bit ) & e->_class_flags[ i ] )
        {
          if ( flags_s.tellp() )
          {
            flags_s << ", ";
          }

          flags_s << ( i * 32 + bit );
        }
      }
    }

    if ( flags_s.tellp() )
    {
      s << "                   Family Flags: ";
      s << flags_s.str();
      s << std::endl;
    }
  }

  if ( e -> _hotfix != 0 )
  {
    s << "                   Hotfixed: ";
    s << effect_hotfix_map_str( e );
    s << std::endl;
  }


  s.precision( ssize );

  return s;
}

std::string spell_info::to_str( const dbc_t& dbc, const spell_data_t* spell, int level )
{

  std::ostringstream s;
  player_e pt = PLAYER_NONE;

  if ( spell -> scaling_class() != 0 && spell -> level() > static_cast< unsigned >( level ) )
  {
    s << std::endl << "Too low spell level " << level << " for " << spell -> name_cstr() << ", minimum is " << spell -> level() << "." << std::endl << std::endl;
    return s.str();
  }

  std::string name_str = spell -> name_cstr();
  if ( spell -> rank_str() )
    name_str += " (rank=" + std::string( spell -> rank_str() ) + ")";
  s <<   "Name             : " << name_str << " (id=" << spell -> id() << ") " << spell_flags( spell ) << std::endl;

  if ( spell -> _hotfix != 0 )
  {
    auto hotfix_str = spell_hotfix_map_str( spell );
    if ( ! hotfix_str.empty() )
    {
      s << "Hotfixed         : " << hotfix_str << std::endl;
    }
  }

  if ( spell -> replace_spell_id() > 0 )
  {
    s << "Replaces         : " <<  dbc.spell( spell -> replace_spell_id() ) -> name_cstr();
    s << " (id=" << spell -> replace_spell_id() << ")" << std::endl;
  }

  if ( spell -> class_mask() )
  {
    bool pet_ability = false;
    s << "Class            : ";

    if ( dbc.is_specialization_ability( spell -> id() ) )
    {
      std::vector<specialization_e> spec_list;
      std::vector<specialization_e>::const_iterator iter;
      dbc.ability_specialization( spell -> id(), spec_list );

      for ( iter = spec_list.begin(); iter != spec_list.end(); ++iter )
      {
        if ( *iter == PET_FEROCITY || *iter == PET_CUNNING || *iter == PET_TENACITY )
          pet_ability = true;
        s << util::inverse_tokenize( dbc::specialization_string( *iter ) ) << " ";
      }
      spec_list.clear();
    }

    for ( unsigned int i = 1; i < sizeof_array( _class_map ); i++ )
    {
      if ( ( spell -> class_mask() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
      {
        s << _class_map[ i ].name << ", ";
        if ( ! pt )
          pt = _class_map[ i ].pt;
      }
    }

    s.seekp( -2, std::ios_base::cur );
    if ( pet_ability )
      s << " Pet";
    s << std::endl;
  }

  if ( spell -> race_mask() )
  {
    s << "Race             : ";

    for ( unsigned int i = 0; i < sizeof( spell -> race_mask() ) * 8; i++ )
    {
      uint64_t mask = (uint64_t(1) << i );
      if ( ( spell -> race_mask() & mask ) && _race_map.find( i ) != _race_map.end() )
      {
        auto it = _race_map.find( i );
        if ( it != _race_map.end() )
        {
          s << it -> second << ", ";
        }
      }
    }

    s.seekp( -2, std::ios_base::cur );
    s << " (0x" << std::hex << spell -> race_mask() << std::dec << ")";
    s << std::endl;
  }

  std::string school_string = util::school_type_string( spell -> get_school_type() );
  school_string[ 0 ] = std::toupper( school_string[ 0 ] );
  s << "School           : " << school_string << std::endl;

  std::string spell_type_str = "Unknown(" + util::to_string( spell->dmg_class() ) + ")";
  if ( spell->dmg_class() < _spell_type_map.size() )
  {
    spell_type_str = _spell_type_map[ spell->dmg_class() ];
  }
  s << "Spell Type       : " << spell_type_str << std::endl;

  for ( size_t i = 0; spell -> _power && i < spell -> _power -> size(); i++ )
  {
    const spellpower_data_t* pd = spell -> _power -> at( i );

    s << "Resource         : ";

    if ( pd -> type() == POWER_MANA )
      s << pd -> cost() * 100.0 << "%";
    else
      s << pd -> cost();

    s << " ";

    if ( pd -> max_cost() != 0 )
    {
      s << "- ";
      if ( pd -> type() == POWER_MANA )
        s << ( pd -> cost() + pd -> max_cost() ) * 100.0 << "%";
      else
        s << ( pd -> cost() + pd -> max_cost() );
      s << " ";
    }

    s << map_string( _resource_strings, pd -> raw_type() );

    if ( pd -> cost_per_tick() != 0 )
    {
      s << " and ";

      if ( pd -> type() == POWER_MANA )
        s << pd -> cost_per_tick() * 100.0 << "%";
      else
        s << pd -> cost_per_tick();

      s << " " << map_string( _resource_strings, pd -> raw_type() ) << " per tick";
    }

    s << " (id=" << pd -> id() << ")";

    if ( pd -> aura_id() > 0 && dbc.spell( pd -> aura_id() ) -> id() == pd -> aura_id() )
      s << " w/ " << dbc.spell( pd -> aura_id() ) -> name_cstr() << " (id=" << pd -> aura_id() << ")";

    if ( pd -> _hotfix != 0 )
    {
      auto hotfix_str = power_hotfix_map_str( pd );
      if ( ! hotfix_str.empty() )
      {
        s << " [Hotfixed: " << hotfix_str << "]";
      }
    }

    s << std::endl;
  }

  if ( spell -> level() > 0 )
  {
    s << "Spell Level      : " << ( int ) spell -> level();
    if ( spell -> max_level() > 0 )
      s << " (max " << ( int ) spell -> max_level() << ")";

    s << std::endl;
  }

  if ( spell -> max_scaling_level() > 0 )
  {
    s << "Max Scaling Level: " << ( int ) spell -> max_scaling_level();
    s << std::endl;
  }

  if ( spell -> req_max_level() > 0 )
  {
    s << "Req. Max Level   : " << ( int ) spell -> req_max_level();
    s << std::endl;
  }

  if ( spell -> min_range() || spell -> max_range() )
  {
    s << "Range            : ";
    if ( spell -> min_range() )
      s << ( int ) spell -> min_range() << " - ";

    s << ( int ) spell -> max_range() << " yards" << std::endl;
  }

  if ( spell -> _cast_min > 0 || spell -> _cast_max > 0 )
  {
    s << "Cast Time        : ";

    if ( spell -> _cast_div )
      s << spell -> cast_time( level ).total_seconds();
    else if ( spell -> _cast_min != spell -> _cast_max )
      s << spell -> _cast_min / 1000.0 << " - " << spell -> _cast_max / 1000.0;
    else
      s << spell -> _cast_max / 1000.0;

    s << " seconds" << std::endl;
  }
  else if ( spell -> _cast_min < 0 || spell -> _cast_max < 0 )
    s << "Cast Time        : Ranged Shot" << std::endl;

  if ( spell -> gcd() != timespan_t::zero() )
    s << "GCD              : " << spell -> gcd().total_seconds() << " seconds" << std::endl;

  if ( spell -> missile_speed() )
    s << "Velocity         : " << spell -> missile_speed() << " yards/sec"  << std::endl;

  if ( spell -> duration() != timespan_t::zero() )
  {
    s << "Duration         : ";
    if ( spell -> duration() < timespan_t::zero() )
      s << "Aura (infinite)";
    else
      s << spell -> duration().total_seconds() << " seconds";

    s << std::endl;
  }

  if ( spell->equipped_class() == ITEM_CLASS_WEAPON )
  {
    std::vector<std::string> weapon_types;
    for ( auto wt = ITEM_SUBCLASS_WEAPON_AXE; wt < ITEM_SUBCLASS_WEAPON_FISHING_POLE; ++wt )
    {
      if ( spell->equipped_subclass_mask() & ( 1 << static_cast<unsigned>( wt ) ) )
      {
        weapon_types.push_back( util::weapon_subclass_string( wt ) );
      }
    }
    s << "Requires weapon  : ";
    if ( weapon_types.size() > 0 )
    {
      s << util::string_join( weapon_types );
    }
    s << std::endl;
  }

  if ( spell -> cooldown() > timespan_t::zero() )
    s << "Cooldown         : " << spell -> cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell -> charges() > 0 || spell -> charge_cooldown() > timespan_t::zero() )
  {
    s << "Charges          : " << spell -> charges();
    if ( spell -> charge_cooldown() > timespan_t::zero() )
      s << " (" << spell -> charge_cooldown().total_seconds() << " seconds cooldown)";
    s << std::endl;
  }

  if ( spell -> category() > 0 )
  {
    s << "Category         : " << spell -> category();
    auto affecting_effects = dbc.effect_categories_affecting_spell( spell );
    if ( affecting_effects.size() > 0 )
    {
      s << ": ";
      s << concatenate<spelleffect_data_t>( affecting_effects,
        []( std::stringstream& s, const spelleffect_data_t* e ) {
          s << e -> spell() -> name_cstr() << " (" << e -> spell() -> id() << " effect#" << ( e -> index() + 1 ) << ")";
        } );
    }
    s << std::endl;
  }

  if ( spell -> label_count() > 0 )
  {
    s << "Labels           : ";
    for ( size_t i = 1, end = spell -> label_count(); i <= end; ++i )
    {
      auto label = spell -> labelN( i );
      auto affecting_effects = dbc.effect_labels_affecting_label( label );

      if ( i > 1 )
      {
        if ( affecting_effects.size() == 0 )
        {
          if ( i < end )
          {
            s << ", ";
          }
        }
        else
        {
          s << std::endl;
          s << "                 : ";
        }
      }

      s << label;

      if ( affecting_effects.size() > 0 )
      {
        s << ": " << concatenate<spelleffect_data_t>( affecting_effects,
          []( std::stringstream& s, const spelleffect_data_t* e ) {
            s << e -> spell() -> name_cstr() << " (" << e -> spell() -> id()
              << " effect#" << ( e -> index() + 1 ) << ")";
          } );
      }
    }
    s << std::endl;
  }

  if ( spell -> category_cooldown() > timespan_t::zero() )
    s << "Category Cooldown: " << spell -> category_cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell -> internal_cooldown() > timespan_t::zero() )
    s << "Internal Cooldown: " << spell -> internal_cooldown().total_seconds() << " seconds" << std::endl;

  if ( spell -> initial_stacks() > 0 || spell -> max_stacks() )
  {
    s << "Stacks           : ";
    if ( spell -> initial_stacks() )
      s << spell -> initial_stacks() << " initial, ";

    if ( spell -> max_stacks() )
      s << spell -> max_stacks() << " maximum, ";
    else if ( spell -> initial_stacks() && ! spell -> max_stacks() )
      s << spell -> initial_stacks() << " maximum, ";

    s.seekp( -2, std::ios_base::cur );
  
    s << std::endl;
  }

  if ( spell -> proc_chance() > 0 )
    s << "Proc Chance      : " << spell -> proc_chance() * 100 << "%" << std::endl;

  if ( spell -> real_ppm() != 0 )
  {
    s << "Real PPM         : " << spell -> real_ppm();
    bool has_modifiers = false;
    std::vector<const rppm_modifier_t*> modifiers = dbc.real_ppm_modifiers( spell -> id() );
    for ( size_t i = 0; i < modifiers.size(); ++i )
    {
      const rppm_modifier_t* rppm_modifier = modifiers[ i ];

      switch ( rppm_modifier -> modifier_type )
      {
        case RPPM_MODIFIER_HASTE:
          if ( ! has_modifiers )
          {
            s << " (";
          }
          s << "Haste multiplier, ";
          has_modifiers = true;
          break;
        case RPPM_MODIFIER_CRIT:
          if ( ! has_modifiers )
          {
            s << " (";
          }
          s << "Crit multiplier, ";
          has_modifiers = true;
          break;
        case RPPM_MODIFIER_ILEVEL:
          if ( ! has_modifiers )
          {
            s << " (";
          }
          s << "Itemlevel multiplier [base=" << rppm_modifier -> type << "], ";
          has_modifiers = true;
          break;
        case RPPM_MODIFIER_SPEC:
        {
          if ( ! has_modifiers )
          {
            s << " (";
          }

          std::streamsize decimals = 3;
          double rppm_val = spell -> real_ppm() * ( 1.0 + rppm_modifier -> coefficient );
          if ( rppm_val >= 10 )
            decimals += 2;
          else if ( rppm_val >= 1 )
            decimals += 1;
          s.precision( decimals );
          s << util::specialization_string( static_cast<specialization_e>( rppm_modifier -> type ) ) << ": " << rppm_val << ", ";
          has_modifiers = true;
          break;
        }
        default:
          break;
      }
    }

    if ( has_modifiers )
    {
      s.seekp( -2, std::ios_base::cur );
      s << ")";
    }
    s << std::endl;
  }

  if ( spell -> stance_mask() > 0 )
  {
    s << "Stance Mask      : 0x";
    std::streamsize ss = s.width();
    s.width( 8 );
    s << std::hex << std::setfill('0') << spell -> stance_mask() << std::endl << std::dec;
    s.width( ss );
  }

  if ( spell -> mechanic() > 0 )
  {
    s << "Mechanic         : " << mechanic_str( spell -> mechanic() ) << std::endl;
  }

  if ( spell -> power_id() > 0 )
  {
    s << "Azerite Power Id : " << spell -> power_id() << std::endl;
  }

  if ( spell->essence_id() > 0 )
  {
    s << "Azerite EssenceId: " << spell->essence_id() << " ";

    const auto data = azerite_essence_power_entry_t::data_by_essence_id( spell->essence_id(), dbc.ptr );

    s << azerite_essence_str( spell, data );
    s << std::endl;
  }

  if ( spell -> proc_flags() > 0 )
  {
    s << "Proc Flags       : ";
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( spell -> proc_flags() & ( 1 << flag ) )
        s << "x";
      else
        s << ".";

      if ( ( flag + 1 ) % 8 == 0 )
        s << " ";
    }
    s << std::endl;
    s << "                 : ";
    for ( size_t i = 0; i < sizeof_array( _proc_flag_map ); i++ )
    {
      if ( spell -> proc_flags() & _proc_flag_map[ i ].flag )
      {
        s << _proc_flag_map[ i ].proc;
        s << ", ";
      }
    }
    std::streampos x = s.tellp();
    s.seekp( x - std::streamoff( 2 ) );
    s << std::endl;
  }

  if ( spell -> class_family() > 0 )
  {
    auto affecting_effects = dbc.effects_affecting_spell( spell );
    std::vector<unsigned> spell_ids;
    if ( affecting_effects.size() > 0 )
    {
      s << "Affecting spells : ";

      for ( size_t i = 0, end = affecting_effects.size(); i < end; i++ )
      {
        const spelleffect_data_t* effect = affecting_effects[ i ];
        if ( std::find( spell_ids.begin(), spell_ids.end(), effect -> spell() -> id() ) != spell_ids.end() )
          continue;

        s << effect -> spell() -> name_cstr() << " (" << effect -> spell() -> id() << " effect#"
          << ( effect -> index() + 1 ) << ")";
        if ( i < end - 1 )
          s << ", ";

        spell_ids.push_back( effect -> spell() -> id() );
      }

      s << std::endl;
    }
  }

  if ( spell -> _driver )
  {
    s << "Triggered by     : ";
    for ( size_t driver_idx = 0; driver_idx < spell -> _driver -> size(); ++driver_idx )
    {
      const spell_data_t* driver = spell -> _driver -> at( driver_idx );
      s << driver -> name_cstr() << " (" << driver -> id() << ")";
      if ( driver_idx < spell -> _driver -> size() - 1 )
      {
        s << ", ";
      }
    }
    s << std::endl;
  }

  if ( spell->class_family() > 0 )
  {
    std::stringstream flags_s;

    for ( size_t i = 0; i < NUM_CLASS_FAMILY_FLAGS; ++i )
    {
      for ( size_t bit = 0; bit < 32; ++bit )
      {
        if ( ( 1 << bit ) & spell->_class_flags[ i ] )
        {
          if ( flags_s.tellp() )
          {
            flags_s << ", ";
          }

          flags_s << ( i * 32 + bit );
        }
      }
    }

    if ( flags_s.tellp() )
    {
      s << "Family Flags     : ";
      s << flags_s.str();
      s << std::endl;
    }
  }

  s << "Attributes       : ";
  std::string attr_str;
  for ( unsigned i = 0; i < NUM_SPELL_FLAGS; i++ )
  {
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( spell -> attribute( i ) & ( 1 << flag ) )
      {
        s << "x";
        size_t attr_idx = i * 32 + flag;
        auto it = _attribute_strings.find( static_cast<unsigned int>( attr_idx ) );
        if ( it != _attribute_strings.end() )
        {
          if ( ! attr_str.empty() )
            attr_str += ", ";

          attr_str += it -> second;
          attr_str += " (" + util::to_string( attr_idx ) + ")";
        }
      }
      else
        s << ".";

      if ( ( flag + 1 ) % 8 == 0 )
        s << " ";

      if ( flag == 31 && i % 2 == 0 )
        s << "  ";
    }

    if ( ( i + 1 ) % 2 == 0 && i < NUM_SPELL_FLAGS - 1 )
      s << std::endl << "                 : ";
  }

  if ( ! attr_str.empty() )
      s << std::endl << "                 : " + attr_str;
  s << std::endl;

  s << "Effects          :" << std::endl;

  for ( size_t i = 0; i < spell -> effect_count(); i++ )
  {
    const spelleffect_data_t* e;
    uint32_t effect_id;
    if ( ! ( effect_id = spell -> effectN( i + 1 ).id() ) )
      continue;
    else
      e = &( spell -> effectN( i + 1 ) );

    spell_info::effect_to_str( dbc, spell, e, s, level );
  }

  if ( spell -> desc() )
    s << "Description      : " << spell -> desc() << std::endl;

  if ( spell -> tooltip() )
    s << "Tooltip          : " << spell -> tooltip() << std::endl;

  if ( spell -> desc_vars() )
    s << "Variables        : " << spell -> desc_vars() << std::endl;

  s << std::endl;

  return s.str();
}

std::string spell_info::talent_to_str( const dbc_t& /* dbc */, const talent_data_t* talent, int /* level */ )
{
  std::ostringstream s;

  s <<   "Name         : " << talent -> name_cstr() << " (id=" << talent -> id() << ") " << std::endl;

  if ( talent -> mask_class() )
  {
    s << "Class        : ";
    for ( unsigned int i = 1; i < sizeof_array( _class_map ); i++ )
    {
      if ( ( talent -> mask_class() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
        s << _class_map[ i ].name << ", ";
    }

    s.seekp( -2, std::ios_base::cur );
    s << std::endl;
  }

  s << "Column       : " << talent -> col() + 1    << std::endl;
  s << "Row          : " << talent -> row() + 1    << std::endl;
  s << "Spell        : "        << talent -> spell_id()   << std::endl;
  if ( talent -> replace_id() > 0 )
    s << "Replaces     : "   << talent -> replace_id() << std::endl;
  s << "Spec         : " << util::specialization_string( talent -> specialization() ) << std::endl;
  s << std::endl;

  return s.str();
}

std::string spell_info::set_bonus_to_str( const dbc_t&, const item_set_bonus_t* set_bonus, int /* level */ )
{
  std::ostringstream s;

  s << "Name          : " << set_bonus -> set_name << std::endl;

  player_e player_type = static_cast<player_e>( set_bonus -> class_id);
  s << "Class         : " << util::player_type_string( player_type ) << std::endl;
  s << "Tier          : " << set_bonus -> tier << std::endl;
  s << "Bonus Level   : " << set_bonus -> bonus << std::endl;
  if ( set_bonus -> spec > 0 )
    s << "Spec          : " << util::specialization_string( static_cast<specialization_e>(set_bonus -> spec ) ) << std::endl;
  s << "Spell ID      : " << set_bonus -> spell_id << std::endl;

  s << std::endl;

  return s.str();
}

void spell_info::effect_to_xml( const dbc_t& dbc,
                                const spell_data_t*       spell,
                                const spelleffect_data_t* e,
                                xml_node_t* parent,
                                int level )
{
  xml_node_t* node = parent -> add_child( "effect" );

  node -> add_parm( "number", e -> index() + 1 );
  node -> add_parm( "id", e -> id() );
  node -> add_parm( "type", e -> type() );

  if ( map_has_string( _effect_type_strings, e -> raw_type() ) )
  {
    node -> add_parm( "type_text", map_string( _effect_type_strings, e -> raw_type() ) );
  }

  // Put some nice handling on some effect types
  switch ( e -> type() )
  {
    case E_SCHOOL_DAMAGE:
      node -> add_parm( "school", spell -> get_school_type() );
      node -> add_parm( "school_text", util::school_type_string( spell -> get_school_type() ) );
      break;
    case E_TRIGGER_SPELL:
    case E_TRIGGER_SPELL_WITH_VALUE:
      if ( e -> trigger_spell_id() )
      {
        if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
          node -> add_parm( "trigger_spell_name", dbc.spell( e -> trigger_spell_id() ) -> name_cstr() );
      }
      break;
    default:
      break;
  }

  node -> add_parm( "sub_type", e -> subtype() );

  if ( e -> subtype() > 0 )
  {
    node -> add_parm( "sub_type_text", map_string( _effect_subtype_strings, e -> raw_subtype() ) );

    switch ( e -> subtype() )
    {
      case A_PERIODIC_DAMAGE:
        node -> add_parm( "school", spell -> get_school_type() );
        node -> add_parm( "school_text", util::school_type_string( spell -> get_school_type() ) );
        if ( e -> period() != timespan_t::zero() )
          node -> add_parm( "period", e -> period().total_seconds() );
        break;
      case A_PERIODIC_ENERGIZE:
      case A_PERIODIC_DUMMY:
        if ( e -> period() != timespan_t::zero() )
          node -> add_parm( "period", e -> period().total_seconds() );
        break;
      case A_PROC_TRIGGER_SPELL:
        if ( e -> trigger_spell_id() )
        {
          if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
            node -> add_parm( "trigger_spell_name", dbc.spell( e -> trigger_spell_id() ) -> name_cstr() );
        }
        break;
      case A_PERIODIC_TRIGGER_SPELL:
        if ( e -> trigger_spell_id() )
        {
          if ( dbc.spell( e -> trigger_spell_id() ) != spell_data_t::nil() )
          {
            node -> add_parm( "trigger_spell_name", dbc.spell( e -> trigger_spell_id() ) -> name_cstr() );
            if ( e -> period() != timespan_t::zero() )
              node -> add_parm( "period", e -> period().total_seconds() );
          }
        }
        break;
      case A_ADD_FLAT_MODIFIER:
      case A_ADD_PCT_MODIFIER:
        node -> add_parm( "modifier", e -> misc_value1() );
        if ( map_has_string( _property_type_strings, e -> misc_value1() ) )
        {
          node -> add_parm( "modifier_text", map_string( _property_type_strings, e -> misc_value1() ) );
        }
        break;
      default:
        break;
    }
  }
  node -> add_parm( "base_value", e -> base_value() );

  if ( level <= MAX_LEVEL )
  {
    double v_min = dbc.effect_min( e -> id(), level );
    double v_max = dbc.effect_max( e -> id(), level );
    node -> add_parm( "scaled_value", v_min  );
    if ( v_min != v_max )
    {
      node -> add_parm( "scaled_value_max", v_max );
    }
  }
  else
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( level );
    double item_budget = ilevel_data.p_epic[ 0 ];

    node -> add_parm( "scaled_value", item_budget * e -> m_coefficient() );
  }

  if ( e -> m_coefficient() != 0 )
  {
    node -> add_parm( "multiplier_coefficient", e -> m_coefficient() );
  }

  if ( e -> m_delta() != 0 )
  {
    node -> add_parm( "multiplier_delta", e -> m_delta() );
  }

  if ( level <= MAX_LEVEL )
  {
    if ( e -> m_unk() )
    {
      node -> add_parm( "bonus_value", dbc.effect_bonus( e -> id(), level ) );
      node -> add_parm( "bonus_value_multiplier", e -> m_unk() );
    }
  }

  if ( e -> real_ppl() != 0 )
  {
    node -> add_parm( "points_per_level", e -> real_ppl() );
  }

  if ( e -> sp_coeff() != 0 )
  {
    node -> add_parm( "sp_coefficient", e -> sp_coeff() );
  }

  if ( e -> ap_coeff() != 0 )
  {
    node -> add_parm( "ap_coefficient", e -> ap_coeff() );
  }

  if ( e -> chain_multiplier() != 0 && e -> chain_multiplier() != 1.0 )
    node -> add_parm( "chain_multiplier", e -> chain_multiplier() );

  if ( e -> misc_value1() != 0 || e -> type() == E_ENERGIZE )
  {
    if ( e -> subtype() == A_MOD_DAMAGE_DONE ||
         e -> subtype() == A_MOD_DAMAGE_TAKEN ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_DONE ||
         e -> subtype() == A_MOD_DAMAGE_PERCENT_TAKEN )
      node -> add_parm( "misc_value_mod_damage", e -> misc_value1() );
    else if ( e -> type() == E_ENERGIZE )
      node -> add_parm( "misc_value_energize", util::resource_type_string( util::translate_power_type( static_cast<power_e>( e -> misc_value1() ) ) ) );
    else
      node -> add_parm( "misc_value", e -> misc_value1() );
  }

  if ( e -> misc_value2() != 0 )
  {
    node -> add_parm( "misc_value_2", e -> misc_value2() );
  }

  if ( e -> pp_combo_points() != 0 )
    node -> add_parm( "points_per_combo_point", e -> pp_combo_points() );

  if ( e -> trigger_spell_id() != 0 )
    node -> add_parm( "trigger_spell_id", e -> trigger_spell_id() );
}

void spell_info::to_xml( const dbc_t& dbc, const spell_data_t* spell, xml_node_t* parent, int level )
{
  player_e pt = PLAYER_NONE;

  if ( spell -> scaling_class() != 0 && spell -> level() > static_cast< unsigned >( level ) )
  {
    return;
  }

  xml_node_t* node = parent -> add_child( "spell" );

  node -> add_parm( "id", spell -> id() );
  node -> add_parm( "name", spell -> name_cstr() );
  spell_flags_xml( spell, node );

  if ( spell -> replace_spell_id() > 0 )
  {
    node -> add_parm( "replaces_name", dbc.spell( spell -> replace_spell_id() ) -> name_cstr() );
    node -> add_parm( "replaces_id", spell -> replace_spell_id() );
  }

  if ( spell -> class_mask() )
  {
    bool pet_ability = false;

    if ( dbc.is_specialization_ability( spell -> id() ) )
    {
      std::vector<specialization_e> spec_list;
      std::vector<specialization_e>::iterator iter;
      dbc.ability_specialization( spell -> id(), spec_list );

      for ( iter = spec_list.begin(); iter != spec_list.end(); ++iter )
      {
        xml_node_t* spec_node = node -> add_child( "spec" );
        spec_node -> add_parm( "id", *iter );
        spec_node -> add_parm( "name", dbc::specialization_string( *iter ) );
        if ( *iter == PET_FEROCITY || *iter == PET_CUNNING || *iter == PET_TENACITY )
        {
          pet_ability = true;
        }
      }
      spec_list.clear();
    }

    for ( unsigned int i = 1; i < sizeof_array( _class_map ); i++ )
    {
      if ( ( spell -> class_mask() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
      {
        xml_node_t* class_node = node -> add_child( "class" );
        class_node -> add_parm( "id", _class_map[ i ].pt );
        class_node -> add_parm( "name", _class_map[ i ].name );
        if ( ! pt )
          pt = _class_map[ i ].pt;
      }
    }

    if ( pet_ability )
      node -> add_child( "class" ) -> add_parm( ".",  "Pet" );
  }

  if ( spell -> race_mask() )
  {
    for ( unsigned int i = 0; i < sizeof( spell -> race_mask() ) * 8; i++ )
    {
      uint64_t mask = (uint64_t(1) << i );
      if ( ( spell -> race_mask() & mask ) )
      {
        auto it = _race_map.find( i );
        if ( it != _race_map.end() )
        {
          xml_node_t* race_node = node -> add_child( "race" );
          race_node -> add_parm( "id", i );
          race_node -> add_parm( "name", it -> second );
        }
      }
    }
  }

  for ( size_t i = 0; spell -> _power && i < spell -> _power -> size(); i++ )
  {
    const spellpower_data_t* pd = spell -> _power -> at( i );

    if ( pd -> cost() == 0 )
      continue;

    xml_node_t* resource_node = node -> add_child( "resource" );
    resource_node -> add_parm( "type", ( signed ) pd -> type() );

    if ( pd -> type() == POWER_MANA )
      resource_node -> add_parm( "cost", spell -> cost( pd -> type() ) * 100.0 );
    else
      resource_node -> add_parm( "cost", spell -> cost( pd -> type() ) );

    if ( map_has_string( _resource_strings, pd -> raw_type() ) )
    {
      resource_node -> add_parm( "type_name", map_string( _resource_strings, pd -> raw_type() ) );
    }

    if ( pd -> type() == POWER_MANA )
    {
      resource_node -> add_parm( "cost_mana_flat", floor( dbc.resource_base( pt, level ) * pd -> cost() ) );
      resource_node -> add_parm( "cost_mana_flat_level", level );
    }

    if ( pd -> aura_id() > 0 && dbc.spell( pd -> aura_id() ) -> id() == pd -> aura_id() )
    {
      resource_node -> add_parm( "cost_aura_id", pd -> aura_id() );
      resource_node -> add_parm( "cost_aura_name", dbc.spell( pd -> aura_id() ) -> name_cstr() );
    }
  }

  if ( spell -> level() > 0 )
  {
    node -> add_parm( "level", spell -> level() );
    if ( spell -> max_level() > 0 )
      node -> add_parm( "max_level", spell -> max_level() );
  }

  if ( spell -> min_range() || spell -> max_range() )
  {
    if ( spell -> min_range() )
      node -> add_parm( "range_min", spell -> min_range() );
    node -> add_parm( "range", spell -> max_range() );
  }

  if ( spell -> _cast_min > 0 || spell -> _cast_max > 0 )
  {
    if ( spell -> _cast_div )
      node -> add_parm( "cast_time", spell -> cast_time( level ).total_seconds() );
    else if ( spell -> _cast_min != spell -> _cast_max )
    {
      node -> add_parm( "cast_time_min", spell -> _cast_min / 1000.0 );
      node -> add_parm( "cast_time_max", spell -> _cast_max / 1000.0 );
    }
    else
      node -> add_parm( "cast_time_else", spell -> _cast_max / 1000.0 );
  }
  else if ( spell -> _cast_min < 0 || spell -> _cast_max < 0 )
    node -> add_parm( "cast_time_range", "ranged_shot" );

  if ( spell -> gcd() != timespan_t::zero() )
    node -> add_parm( "gcd", spell -> gcd().total_seconds() );

  if ( spell -> missile_speed() )
    node -> add_parm( "velocity", spell -> missile_speed() );

  if ( spell -> duration() != timespan_t::zero() )
  {
    if ( spell -> duration() < timespan_t::zero() )
      node -> add_parm( "duration", "-1" );
    else
      node -> add_parm( "duration", spell -> duration().total_seconds() );
  }

  if ( spell -> cooldown() > timespan_t::zero() )
    node -> add_parm( "cooldown", spell -> cooldown().total_seconds() );

  if ( spell -> initial_stacks() > 0 || spell -> max_stacks() )
  {
    if ( spell -> initial_stacks() )
      node -> add_parm( "stacks_initial", spell -> initial_stacks() );

    if ( spell -> max_stacks() )
      node -> add_parm( "stacks_max", spell -> max_stacks() );
    else if ( spell -> initial_stacks() && ! spell -> max_stacks() )
      node -> add_parm( "stacks_max", spell -> initial_stacks() );
  }

  if ( spell -> proc_chance() > 0 )
    node -> add_parm( "proc_chance", spell -> proc_chance() * 100 ); // NP 101 % displayed

  if ( spell -> extra_coeff() > 0 )
    node -> add_parm( "extra_coefficient", spell -> extra_coeff() );

  std::string attribs;
  for ( unsigned i = 0; i < NUM_SPELL_FLAGS; i++ )
  {
    for ( unsigned flag = 0; flag < 32; flag++ )
    {
      if ( spell -> _attributes[ i ] & ( 1 << flag ) )
        attribs += "1";
      else
        attribs += "0";
    }
  }
  node -> add_child( "attributes" ) -> add_parm ( ".", attribs );

  xml_node_t* effect_node = node -> add_child( "effects" );
  effect_node -> add_parm( "count", spell -> _effects -> size() );

  for ( size_t i = 0; i < spell -> _effects -> size(); i++ )
  {
    uint32_t effect_id;
    const spelleffect_data_t* e;
    if ( ! ( effect_id = spell -> _effects -> at( i ) -> id() ) )
      continue;
    else
      e = dbc.effect( effect_id );

    spell_info::effect_to_xml( dbc, spell, e, effect_node, level );
  }

  if ( spell -> desc() )
    node -> add_child( "description" ) -> add_parm( ".", spell -> desc() );

  if ( spell -> tooltip() )
    node -> add_child( "tooltip" ) -> add_parm( ".", spell -> tooltip() );

  if ( spell -> _desc_vars )
    node -> add_child( "variables" ) -> add_parm( ".", spell -> _desc_vars );
}

void spell_info::talent_to_xml( const dbc_t& /* dbc */, const talent_data_t* talent, xml_node_t* parent, int /* level */ )
{
  xml_node_t* node = parent -> add_child( "talent" );

  node -> add_parm( "name", talent -> name_cstr() );
  node -> add_parm( "id", talent -> id() );

  if ( talent -> mask_class() )
  {
    for ( unsigned int i = 1; i < sizeof_array( _class_map ); i++ )
    {
      if ( ( talent -> mask_class() & ( 1 << ( i - 1 ) ) ) && _class_map[ i ].name )
        node -> add_child( "class" ) -> add_parm( ".",  _class_map[ i ].name );
    }
  }

  node -> add_parm( "column", talent -> col() + 1 );
  node -> add_parm( "row", talent -> row() + 1 );
  node -> add_parm( "spell", talent -> spell_id() );
  if ( talent -> replace_id() > 0 )
    node -> add_parm( "replaces", talent -> replace_id() );
}

void spell_info::set_bonus_to_xml( const dbc_t& /* dbc */, const item_set_bonus_t* set_bonus, xml_node_t* parent, int /* level */ )
{
  xml_node_t* node = parent -> add_child( "set_bonus" );

  player_e player_type = static_cast<player_e>( set_bonus -> class_id);
  node -> add_parm( "name", set_bonus -> set_name );
  node -> add_parm( "class", util::player_type_string( player_type ) );
  node -> add_parm( "tier", set_bonus -> tier );
  node -> add_parm( "bonus_level", set_bonus -> bonus );
  if ( set_bonus -> spec > 0 )
  {
    node -> add_parm( "spec", util::specialization_string( static_cast<specialization_e>( set_bonus -> spec ) ) );
  }
  node -> add_parm( "spell_id", set_bonus -> spell_id );

}
