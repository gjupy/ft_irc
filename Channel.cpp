/* ************************************************************************** */
/*																			*/
/*														:::	  ::::::::   */
/*   CHannel.cpp										:+:	  :+:	:+:   */
/*													+:+ +:+		 +:+	 */
/*   By: gjupy <gjupy@student.42.fr>				+#+  +:+	   +#+		*/
/*												+#+#+#+#+#+   +#+		   */
/*   Created: 2023/04/26 15:50:11 by gjupy			 #+#	#+#			 */
/*   Updated: 2023/04/26 18:04:32 by gjupy			###   ########.fr	   */
/*																			*/
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel()
{
	invite_only = false;
	topic_restriciton = false;
	key = false;
	privilege = false;
	user_limit = false;
}

Channel::~Channel() {}

bool	Channel::get_invite_only() const
{
	return (invite_only);
}

bool	Channel::get_topic_restriciton() const
{
	return (topic_restriciton);
}

bool	Channel::get_key() const
{
	return (key);
}

bool	Channel::get_privilege() const
{
	return (privilege);
}

bool	Channel::get_user_limit() const
{
	return (user_limit);
}

void Channel::set_invite_only(bool value) {
	invite_only = value;
}

void Channel::set_topic_restriciton(bool value) {
	topic_restriciton = value;
}

void Channel::set_key(bool value) {
	key = value;
}

void Channel::set_privilege(bool value) {
	privilege = value;
}

void Channel::set_user_limit(bool value) {
	user_limit = value;
}
