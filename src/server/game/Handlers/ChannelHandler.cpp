/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectMgr.h"                                      // for normalizePlayerName
#include "ChannelMgr.h"
#include "Chat.h"

void WorldSession::HandleJoinChannel(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 channelId;
    uint32 channelLength = 0;
    uint32 passLength = 0;
    std::string channelName, pass;

    recvPacket >> channelId;
    passLength = recvPacket.ReadBits(7);
    uint8 unknown1 = recvPacket.ReadBit();                  // unknown bit
    channelLength = recvPacket.ReadBits(7);
    uint8 unknown2 = recvPacket.ReadBit();                  // unknown bit
    pass = recvPacket.ReadString(passLength);
    channelName = recvPacket.ReadString(channelLength);

    if (channelId)
    {
        ChatChannelsEntry const* channel = sChatChannelsStore.LookupEntry(channelId);
        if (!channel)
            return;

        AreaTableEntry const* current_zone = GetAreaEntryByAreaID(_player->GetZoneId());
        if (!current_zone)
            return;

        if (!_player->CanJoinConstantChannelInZone(channel, current_zone))
            return;
    }

    if (channelName.empty())
        return;

    if (!ChatHandler(this).isValidChatMessage(channelName.c_str()))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        cMgr->team = _player->GetTeam();
        if (Channel* chn = cMgr->GetJoinChannel(channelName, channelId))
            chn->Join(_player->GetGUID(), pass.c_str());
    }
}

void WorldSession::HandleLeaveChannel(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 unk;
    std::string channelname;
    recvPacket >> unk;                                      // channel id?
    uint32 length = recvPacket.ReadBits(7);
    recvPacket.FlushBits();
    channelname = recvPacket.ReadString(length);

    if (channelname.empty())
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Leave(_player->GetGUID(), true);
        cMgr->LeftChannel(channelname);
    }
}

void WorldSession::HandleChannelList(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 length = recvPacket.ReadBits(7);
    recvPacket.FlushBits();
    std::string channelname = recvPacket.ReadString(length);

    if (!ChatHandler(this).isValidChatMessage(channelname.c_str()))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->List(_player);
}

void WorldSession::HandleChannelPassword(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelPassCommand < 5)
        return;
    else
        timeLastChannelPassCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());
    uint32 passLength = recvPacket.ReadBits(6);
    uint32 nameLength = recvPacket.ReadBits(7);

    recvPacket.FlushBits();

    std::string channelname = recvPacket.ReadString(nameLength);
    std::string pass = recvPacket.ReadString(passLength);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Password(_player->GetGUID(), pass.c_str());
}

void WorldSession::HandleChannelSetOwner(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelSetownerCommand < 5)
        return;
    else
       timeLastChannelSetownerCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 channelLength = recvPacket.ReadBits(7);
    uint32 nameLength = recvPacket.ReadBits(6);

    recvPacket.FlushBits();

    std::string newp = recvPacket.ReadString(nameLength);
    std::string channelname = recvPacket.ReadString(channelLength);

    if (!normalizePlayerName(newp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetOwner(_player->GetGUID(), newp.c_str());
}

void WorldSession::HandleChannelOwner(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelOwnerCommand < 5)
        return;
    else
       timeLastChannelOwnerCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 length = recvPacket.ReadBits(7);

    recvPacket.FlushBits();

    std::string channelname = recvPacket.ReadString(length);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SendWhoOwner(_player->GetGUID());
}

void WorldSession::HandleChannelModerator(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelModerCommand < 5)
        return;
    else
       timeLastChannelModerCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 nameLength = recvPacket.ReadBits(7);
    uint32 channelLength = recvPacket.ReadBits(9);

    recvPacket.FlushBits();

    std::string otp = recvPacket.ReadString(nameLength);
    std::string channelname = recvPacket.ReadString(channelLength);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetModerator(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelUnmoderator(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelUnmoderCommand < 5)
        return;
    else
       timeLastChannelUnmoderCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 nameLength = recvPacket.ReadBits(6);
    uint32 channelLength = recvPacket.ReadBits(7);

    recvPacket.FlushBits();

    std::string channelname = recvPacket.ReadString(channelLength);
    std::string otp = recvPacket.ReadString(nameLength);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnsetModerator(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelMute(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelMuteCommand < 5)
        return;
    else
        timeLastChannelMuteCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 channelLength = recvPacket.ReadBits(9);
    uint32 nameLength = recvPacket.ReadBits(7);

    recvPacket.FlushBits();

    std::string channelname = recvPacket.ReadString(channelLength);
    std::string otp = recvPacket.ReadString(nameLength);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->SetMute(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelUnmute(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelUnmuteCommand < 5)
        return;
    else
       timeLastChannelUnmuteCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 channelLength = recvPacket.ReadBits(7);
    uint32 nameLength = recvPacket.ReadBits(6);

    recvPacket.FlushBits();

    std::string channelname = recvPacket.ReadString(channelLength);
    std::string otp = recvPacket.ReadString(nameLength);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnsetMute(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelInvite(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelInviteCommand < 5)
        return;
    else
       timeLastChannelInviteCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());
    uint32 channelLength = recvPacket.ReadBits(7);
    uint32 nameLength = recvPacket.ReadBits(6);

    recvPacket.FlushBits();

    std::string channelname = recvPacket.ReadString(channelLength);
    std::string otp = recvPacket.ReadString(nameLength);

    if (!normalizePlayerName(otp))
        return;

    if (!ChatHandler(this).isValidChatMessage(channelname.c_str()))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Invite(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelKick(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelKickCommand < 5)
        return;
    else
       timeLastChannelKickCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());
    uint32 nameLength = recvPacket.ReadBits(6);
    uint32 channelLength = recvPacket.ReadBits(7);

    recvPacket.FlushBits();

    std::string channelname = recvPacket.ReadString(channelLength);
    std::string otp = recvPacket.ReadString(nameLength);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Kick(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelBan(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelBanCommand < 5)
        return;
    else
       timeLastChannelBanCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());
    uint32 channelLength, nameLength;
    std::string channelname, otp;

    channelLength = recvPacket.ReadBits(7);
    nameLength = recvPacket.ReadBits(6);

    recvPacket.FlushBits();

    otp = recvPacket.ReadString(nameLength);
    channelname = recvPacket.ReadString(channelLength);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Ban(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelUnban(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelUnbanCommand < 5)
        return;
    else
       timeLastChannelUnbanCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 channelLength = recvPacket.ReadBits(7);
    uint32 nameLength = recvPacket.ReadBits(6);

    recvPacket.FlushBits();

    std::string channelname = recvPacket.ReadString(channelLength);
    std::string otp = recvPacket.ReadString(nameLength);

    if (!normalizePlayerName(otp))
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->UnBan(_player->GetGUID(), otp.c_str());
}

void WorldSession::HandleChannelAnnouncements(WorldPacket& recvPacket)
{
    time_t now = time(NULL);
    if (now - timeLastChannelAnnounceCommand < 5)
        return;
    else
       timeLastChannelAnnounceCommand = now;

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());

    uint32 length = recvPacket.ReadBits(7);
    recvPacket.FlushBits();
    std::string channelname = recvPacket.ReadString(length);

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
            chn->Announce(_player->GetGUID());
}

void WorldSession::HandleChannelDisplayListQuery(WorldPacket &recvPacket)
{
    // this should be OK because the 2 function _were_ the same
    HandleChannelList(recvPacket);
}

void WorldSession::HandleGetChannelMemberCount(WorldPacket &recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());
    std::string channelname;
    recvPacket >> channelname;
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        if (Channel* chn = cMgr->GetChannel(channelname, _player))
        {
            WorldPacket data(SMSG_CHANNEL_MEMBER_COUNT, chn->GetName().size()+1+1+4);
            data << chn->GetName();
            data << uint8(chn->GetFlags());
            data << uint32(chn->GetNumPlayers());
            SendPacket(&data);
        }
    }
}

void WorldSession::HandleSetChannelWatch(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Opcode %u", recvPacket.GetOpcode());
    std::string channelname;
    recvPacket >> channelname;
    /*if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
        if (Channel* chn = cMgr->GetChannel(channelName, _player))
            chn->JoinNotify(_player->GetGUID());*/
}
