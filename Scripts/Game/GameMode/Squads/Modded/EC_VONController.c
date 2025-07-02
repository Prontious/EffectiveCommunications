modded class SCR_VONController
{
	override protected bool ActivateVON(notnull SCR_VONEntry entry, EVONTransmitType transmitType = EVONTransmitType.NONE)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();

		PrintFormat("[Modded VON] ActivateVON called by Player ID: %1 with Transmit Type: %2", localPlayerId, transmitType);

		// Core restriction block
		if (transmitType == EVONTransmitType.LONG_RANGE || transmitType == EVONTransmitType.CHANNEL || transmitType == EVONTransmitType.NONE)
		{
			SCR_PlayerControllerGroupComponent playerGroupController = SCR_PlayerControllerGroupComponent.GetLocalPlayerControllerGroupComponent();
			if (!playerGroupController)
			{
				PrintFormat("[Modded VON] [Fallback] No PlayerControllerGroupComponent found for Player ID: %1. Allowing transmission.", localPlayerId);
				return super.ActivateVON(entry, transmitType);
			}

			SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
			if (!groupsMgr)
			{
				PrintFormat("[Modded VON] No Groups Manager Found");
				return super.ActivateVON(entry, transmitType);
			}
			SCR_AIGroup playerGroup = groupsMgr.GetPlayerGroup(localPlayerId);

			if (!playerGroup)
			{
				PrintFormat("[Modded VON] [Blocked] Player ID: %1 is not in a group. Transmission blocked .", localPlayerId);
				return false; // Only block this attempt, do not lock VON
			}

			int groupID = playerGroup.GetGroupID();
			int leaderID = playerGroup.GetLeaderID();

			PrintFormat("[Modded VON] Player ID: %1 is in Group ID: %2. Group Leader ID: %3", localPlayerId, groupID, leaderID);

			SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (!factionMgr)
			{
				PrintFormat("[Modded VON] No Faction Manager Found");
				return super.ActivateVON(entry, transmitType);
			}
			SCR_Faction playerFaction = SCR_Faction.Cast(factionMgr.SGetPlayerFaction(localPlayerId));

			if (!playerFaction)
			{
				PrintFormat("[Modded VON] [Fallback] Could not determine faction for Player ID: %1. Allowing transmission.", localPlayerId);
				return super.ActivateVON(entry, transmitType);
			}

			int factionFreq = playerFaction.GetFactionRadioFrequency();
			PrintFormat("[Modded VON] Player ID: %1 Faction Frequency: %2", localPlayerId, factionFreq);

			SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
			if (radioEntry)
			{
				int currentFreq = radioEntry.GetEntryFrequency();

				PrintFormat("[Modded VON] Player ID: %1 attempting to transmit on Frequency: %2", localPlayerId, currentFreq);

				// Block if player is not leader and is using the platoon frequency
				if (currentFreq == factionFreq && localPlayerId != leaderID)
				{

					PrintFormat("[Modded VON] [Blocked] Player ID: %1 is NOT the group leader (Leader ID: %2). Cannot transmit on platoon frequency: %3", localPlayerId, leaderID, factionFreq);
					return false; // Only block this attempt, do not lock VON
				}
				if (currentFreq == factionFreq) {
						PrintFormat("[Modded VON] [Allowed] Player ID: %1 is the group leader (Leader ID: %2). Transmission Allowed on: %3", localPlayerId, leaderID, factionFreq);
					}
				else
				{				PrintFormat("[Modded VON] [Allowed] Player ID: %1 is using a non-platoon frequency. Transmission allowed.", localPlayerId);
}

				return super.ActivateVON(entry, transmitType);
			}
			else
			{
				PrintFormat("[Modded VON] [Fallback] Entry is not a radio entry. Allowing transmission by default.");
				return super.ActivateVON(entry, transmitType);
			}
		}
		else if (transmitType == EVONTransmitType.DIRECT)
		{
			PrintFormat("[Modded VON] [Allowed] Direct transmission by Player ID: %1.", localPlayerId);
			return super.ActivateVON(entry, transmitType);
		}

		PrintFormat("[Modded VON] [Fallback] Unhandled transmit type: %1. Allowing transmission for Player ID: %2", transmitType, localPlayerId);
		return super.ActivateVON(entry, transmitType);
	}
}
