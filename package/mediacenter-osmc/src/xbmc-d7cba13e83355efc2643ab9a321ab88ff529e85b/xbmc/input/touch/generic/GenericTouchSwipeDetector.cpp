/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <stdlib.h>

#include "GenericTouchSwipeDetector.h"

// maximum time between touch down and up (in nanoseconds)
#define SWIPE_MAX_TIME            500000000
// maxmium swipe distance between touch down and up (in multiples of screen DPI)
#define SWIPE_MIN_DISTANCE        0.5f
// original maximum variance of the touch movement
#define SWIPE_MAX_VARIANCE        0.2f
// tangens of the maximum angle (20 degrees) the touch movement may vary in a
// direction perpendicular to the swipe direction (in radians)
// => tan(20 deg) = tan(20 * M_PI / 180)
#define SWIPE_MAX_VARIANCE_ANGLE  0.36397023f

CGenericTouchSwipeDetector::CGenericTouchSwipeDetector(ITouchActionHandler *handler, float dpi)
  : IGenericTouchGestureDetector(handler, dpi),
    m_directions(TouchMoveDirectionLeft | TouchMoveDirectionRight | TouchMoveDirectionUp | TouchMoveDirectionDown),
    m_swipeDetected(false), m_size(0)
{ }

bool CGenericTouchSwipeDetector::OnTouchDown(unsigned int index, const Pointer &pointer)
{
  if (index >= TOUCH_MAX_POINTERS)
    return false;

  m_size += 1;
  if (m_size > 1)
    return true;

  // reset all values
  m_done = false;
  m_swipeDetected = false;
  m_directions = TouchMoveDirectionLeft | TouchMoveDirectionRight | TouchMoveDirectionUp | TouchMoveDirectionDown;

  return true;
}

bool CGenericTouchSwipeDetector::OnTouchUp(unsigned int index, const Pointer &pointer)
{
  if (index >= TOUCH_MAX_POINTERS)
    return false;

  m_size -= 1;
  if (m_done)
    return false;

  m_done = true;

  // check if a swipe has been detected and if it has a valid direction
  if (!m_swipeDetected || m_directions == TouchMoveDirectionNone)
    return false;

  // check if the swipe has been performed in the proper time span
  if ((pointer.current.time - pointer.down.time) > SWIPE_MAX_TIME)
    return false;

  // calculate the velocity of the swipe
  float velocityX = 0.0f; // number of pixels per second
  float velocityY = 0.0f; // number of pixels per second
  pointer.velocity(velocityX, velocityY, false);

  // call the OnSwipe() callback
  OnSwipe((TouchMoveDirection)m_directions, pointer.down.x, pointer.down.y, pointer.current.x, pointer.current.y, velocityX, velocityY, m_size + 1);
  return true;
}

bool CGenericTouchSwipeDetector::OnTouchMove(unsigned int index, const Pointer &pointer)
{
  if (index >= TOUCH_MAX_POINTERS)
    return false;

  // only handle swipes of moved pointers
  if (index >= m_size || m_done || !pointer.moving)
    return false;

  float deltaXmovement = pointer.current.x - pointer.last.x;
  float deltaYmovement = pointer.current.y - pointer.last.y;

  if (deltaXmovement > 0.0f)
    m_directions &= ~TouchMoveDirectionLeft;
  else if (deltaXmovement < 0.0f)
    m_directions &= ~TouchMoveDirectionRight;

  if (deltaYmovement > 0.0f)
    m_directions &= ~TouchMoveDirectionUp;
  else if (deltaYmovement < 0.0f)
    m_directions &= ~TouchMoveDirectionDown;

  if (m_directions == TouchMoveDirectionNone)
  {
    m_done = true;
    return false;
  }

  float deltaXabs = abs(pointer.current.x - pointer.down.x);
  float deltaYabs = abs(pointer.current.y - pointer.down.y);
  float varXabs = deltaYabs * SWIPE_MAX_VARIANCE_ANGLE + (m_dpi * SWIPE_MAX_VARIANCE) / 2;
  float varYabs = deltaXabs * SWIPE_MAX_VARIANCE_ANGLE + (m_dpi * SWIPE_MAX_VARIANCE) / 2;

  if (m_directions & TouchMoveDirectionLeft)
  {
    // check if the movement went too much in Y direction
    if (deltaYabs > varYabs)
      m_directions &= ~TouchMoveDirectionLeft;
    // check if the movement went far enough in the X direction
    else if (deltaXabs > m_dpi * SWIPE_MIN_DISTANCE)
      m_swipeDetected = true;
  }

  if (m_directions & TouchMoveDirectionRight)
  {
    // check if the movement went too much in Y direction
    if (deltaYabs > varYabs)
      m_directions &= ~TouchMoveDirectionRight;
    // check if the movement went far enough in the X direction
    else if (deltaXabs > m_dpi * SWIPE_MIN_DISTANCE)
      m_swipeDetected = true;
  }
  
  if (m_directions & TouchMoveDirectionUp)
  {
    // check if the movement went too much in X direction
    if (deltaXabs > varXabs)
      m_directions &= ~TouchMoveDirectionUp;
    // check if the movement went far enough in the Y direction
    else if (deltaYabs > m_dpi * SWIPE_MIN_DISTANCE)
      m_swipeDetected = true;
  }

  if (m_directions & TouchMoveDirectionDown)
  {
    // check if the movement went too much in X direction
    if (deltaXabs > varXabs)
      m_directions &= ~TouchMoveDirectionDown;
    // check if the movement went far enough in the Y direction
    else if (deltaYabs > m_dpi * SWIPE_MIN_DISTANCE)
      m_swipeDetected = true;
  }

  if (m_directions == TouchMoveDirectionNone)
  {
    m_done = true;
    return false;
  }
  
  return true;
}

bool CGenericTouchSwipeDetector::OnTouchUpdate(unsigned int index, const Pointer &pointer)
{
  if (index >= TOUCH_MAX_POINTERS)
    return false;

  if (m_done)
    return true;

  return OnTouchMove(index, pointer);
}
