// EffectiveCommunications/Scripts/Game/Components/VoNComponent_Modded.c

modded class SCR_VONController
{
	
	//------------------------------------------------------------------------------------------------
	//! VON activation
	//! \param[in] entry which is being activated
	//! \param[in] transmitType type of entry to be activated 
	//! \return true if activation was successful, false otherwise 
 	override protected bool ActivateVON(notnull SCR_VONEntry entry, EVONTransmitType transmitType = EVONTransmitType.NONE)
	{
		
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		// We only care about blocking radio transmissions, not direct speech. 
		if (transmitType == EVONTransmitType.LONG_RANGE)
		{
			// In Enforce Script, we interact with systems through components.
			// First, get the component that manages the local player's group status.[1]
			SCR_PlayerControllerGroupComponent playerGroupController = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
			if (!playerGroupController)
			{
				Print("[Modded VON] Could not find PlayerControllerGroupComponent. Allowing transmission as a fallback.");
				return super.ActivateVON(entry, transmitType);
			}

			// From that component, get the actual group the player is in.
			// We'll assume the method is GetPlayerGroup() based on the component's purpose.  
			SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup playerGroup = groupsMgr.GetPlayerGroup(localPlayerId);
			if (!playerGroup)
			{
				// Player is not in a group, so they cannot be a leader. 
				Print("[Modded VON] Player is not in a group. Blocking LONG_RANGE transmission.");
				return false;
			}

			// Now that we have the group object, we can get the ID of its leader.[2]
			// The method is likely GetLeaderID() as a counterpart to SetGroupLeader(playerID). 
			int leaderID = playerGroup.GetLeaderID();

			// If the local player's ID does not match the leader's ID, block the transmission. 
			if (localPlayerId!= leaderID)
			{
				super.SetVONDisabled(true);
				Print(string.Format("[Modded VON] Player %1 is not the group leader (Leader is %2). Blocking LONG_RANGE transmission.", localPlayerId, leaderID));
				return false;
			}
			
			Print(string.Format("[Modded VON] Player %1 IS the group leader. Allowing LONG_RANGE transmission.", localPlayerId));
		}
		
		// If the check passes, or for any non-radio transmission, proceed with the original game logic.
		return super.ActivateVON(entry, transmitType);
	}
};
