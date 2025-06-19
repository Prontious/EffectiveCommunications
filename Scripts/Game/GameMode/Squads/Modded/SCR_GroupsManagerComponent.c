// CommandAndControl/Scripts/Game/Components/SCR_GroupsManagerComponent_Modded.c

modded class SCR_GroupsManagerComponent
{
    // Define our custom maximum number of squads.
    private const int MAX_SQUADS_LIMIT = 1;

    //------------------------------------------------------------------------------------------------
    override bool CanCreateNewGroup(notnull Faction newGroupFaction)
    {
		
        // Log the start of the function call
        Print(string.Format("[SCR_GroupsManagerComponent_Modded] CanCreateNewGroup called for Faction: %1", newGroupFaction.GetFactionName()));

        // First, run all of the original game's checks. If they fail, we don't need to do anything else.
        if (!super.CanCreateNewGroup(newGroupFaction))
        {
            Print("[SCR_GroupsManagerComponent_Modded] Original CanCreateNewGroup check failed. Returning false.");
            return false;
        }

        // This rule must be enforced by the server. We let clients think they can create
        // a group, but the server will have the final say and reject the request.
        RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
        if (rpl && rpl.IsProxy())
        {
            Print("[SCR_GroupsManagerComponent_Modded] Running on client (proxy). Allowing creation for client-side prediction.");
            return true;
        }

        // Get a list of all currently existing playable groups.
        array<SCR_AIGroup> allGroups = {};
        GetAllPlayableGroups(allGroups);

        // Log the current number of groups
        Print(string.Format("[SCR_GroupsManagerComponent_Modded] Current playable groups count: %1", allGroups.Count()));
        Print(string.Format("[SCR_GroupsManagerComponent_Modded] Defined MAX_SQUADS_LIMIT: %1", MAX_SQUADS_LIMIT));

        // Our custom logic: check if the number of groups has reached our limit.
        if (allGroups.Count() >= MAX_SQUADS_LIMIT)
        {
            // We have reached the maximum number of squads, so prevent the creation of a new one.
            Print(string.Format("[SCR_GroupsManagerComponent_Modded] Max squads limit (%1) reached. Preventing new group creation for Faction: %2.", MAX_SQUADS_LIMIT, newGroupFaction.GetFactionName()));
            return false;
        }

        // If we are below the limit, allow creation to proceed.
        Print(string.Format("[SCR_GroupsManagerComponent_Modded] Below max squads limit (%1). Allowing new group creation for Faction: %2.", MAX_SQUADS_LIMIT, newGroupFaction.GetFactionName()));
        return true;
    }
};